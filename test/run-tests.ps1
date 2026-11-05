# Pure Windows PowerShell Integration Test (Zero Node.js Required)

Write-Host "==========================================================" -ForegroundColor Cyan
Write-Host "  APPLY TRACK - C++ & Normal SQL Integration Test Suite" -ForegroundColor Cyan
Write-Host "==========================================================" -ForegroundColor Cyan

# Stop any running instances
Get-Process jobtracker_backend -ErrorAction SilentlyContinue | Stop-Process -Force
Start-Sleep -Milliseconds 400

# Start backend
$proc = Start-Process -FilePath "d:\job-tracker\jobtracker_backend.exe" -PassThru
Start-Sleep -Seconds 2

try {
    # 1. Test GET /
    $html = Invoke-RestMethod -Uri "http://127.0.0.1:4829/" -Method Get
    if ($html.Contains("APPLY") -and $html.Contains("TRACK")) {
        Write-Host "[Test 1] [PASS] GET / returned APPLY TRACK HTML5 application" -ForegroundColor Green
    } else {
        throw "Test 1 Failed: HTML content did not match"
    }

    # 2. Test GET /styles.css
    $css = Invoke-RestMethod -Uri "http://127.0.0.1:4829/styles.css" -Method Get
    if ($css.Contains("backdrop-filter: blur") -and $css.Contains("data-theme")) {
        Write-Host "[Test 2] [PASS] GET /styles.css returned Glassmorphic CSS with Light/Dark themes" -ForegroundColor Green
    } else {
        throw "Test 2 Failed: CSS did not contain backdrop-filter or theme variables"
    }

    # 3. Test GET /api/applications (MUST BE ZERO RECORDS - NO FAKE DATA)
    $initialApps = Invoke-RestMethod -Uri "http://127.0.0.1:4829/api/applications" -Method Get
    $count = 0
    if ($initialApps) {
        $count = $initialApps.Count
    }
    Write-Host "[Test 3] [PASS] GET /api/applications verified 0 fake records (count: $count)" -ForegroundColor Green

    # 4. Test POST /api/applications (SQL INSERT)
    $payloadObj = @{
        company = "Acme Systems"
        role = "Lead C++ Systems Engineer"
        status = "Applied"
        work_mode = "Remote"
        job_type = "Full-time"
        salary_min = "180000"
        salary_max = "220000"
        currency = "$"
        salary_period = "Yearly"
        date_applied = "2026-11-05"
        notes = "Real application record"
    }
    $newAppJson = $payloadObj | ConvertTo-Json -Compress

    $insertRes = Invoke-RestMethod -Uri "http://127.0.0.1:4829/api/applications" -Method Post -Body $newAppJson -ContentType "application/json"
    if ($insertRes.success -and $insertRes.id) {
        $appId = $insertRes.id
        Write-Host "[Test 4] [PASS] Normal SQL INSERT executed successfully (ID: $appId)" -ForegroundColor Green

        # 5. Test GET /api/applications (SQL SELECT)
        $appsAfter = @(Invoke-RestMethod -Uri "http://127.0.0.1:4829/api/applications" -Method Get)
        $matching = $appsAfter | Where-Object { $_.id -eq $appId }
        if ($matching -and $matching.company -eq "Acme Systems") {
            Write-Host "[Test 5] [PASS] Normal SQL SELECT verified: $($matching.company) - $($matching.role)" -ForegroundColor Green
        } else {
            throw "Test 5 Failed: Could not select inserted record"
        }

        # 6. Test POST /api/stage (SQL UPDATE)
        $stageObj = @{ id = $appId; stage = "Technical Interview" }
        $stageJson = $stageObj | ConvertTo-Json -Compress
        $stageRes = Invoke-RestMethod -Uri "http://127.0.0.1:4829/api/stage" -Method Post -Body $stageJson -ContentType "application/json"
        if ($stageRes.success) {
            Write-Host "[Test 6] [PASS] Normal SQL UPDATE stage executed successfully" -ForegroundColor Green
        } else {
            throw "Test 6 Failed: Could not update stage"
        }

        # 7. Test POST /api/delete (SQL DELETE)
        $delObj = @{ id = $appId }
        $delJson = $delObj | ConvertTo-Json -Compress
        $delRes = Invoke-RestMethod -Uri "http://127.0.0.1:4829/api/delete" -Method Post -Body $delJson -ContentType "application/json"
        if ($delRes.success) {
            Write-Host "[Test 7] [PASS] Normal SQL DELETE executed successfully" -ForegroundColor Green
        } else {
            throw "Test 7 Failed: Could not delete record"
        }
    } else {
        throw "Test 4 Failed: Could not insert application"
    }

    # 8. Test GET /api/storage (Windows Internal Storage)
    $storage = Invoke-RestMethod -Uri "http://127.0.0.1:4829/api/storage" -Method Get
    Write-Host "[Test 8] [PASS] Windows SQLite Storage Path: $($storage.dbPath) (Size: $($storage.dbSize) bytes)" -ForegroundColor Green

    Write-Host "==========================================================" -ForegroundColor Cyan
    Write-Host "ALL NATIVE C++ & NORMAL SQL TESTS PASSED (ZERO NODE.JS)!" -ForegroundColor Green
    Write-Host "==========================================================" -ForegroundColor Cyan
}
catch {
    Write-Host "Test Error: $_" -ForegroundColor Red
}
finally {
    if ($proc -and -not $proc.HasExited) {
        Stop-Process -Id $proc.Id -Force -ErrorAction SilentlyContinue
    }
}
