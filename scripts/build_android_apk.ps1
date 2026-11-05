$ErrorActionPreference = "Stop"

$sdkDir = "C:\Users\Dell\AppData\Local\Android\Sdk"
$buildToolsDir = "$sdkDir\build-tools\35.0.0"
$platformJar = "$sdkDir\platforms\android-35\android.jar"
$jdkBin = "C:\Program Files\Microsoft\jdk-17.0.21.21-hotspot\bin"

$aapt2 = "$buildToolsDir\aapt2.exe"
$d8 = "$buildToolsDir\d8.bat"
$zipalign = "$buildToolsDir\zipalign.exe"
$apksigner = "$buildToolsDir\apksigner.bat"
$javac = "$jdkBin\javac.exe"
$jar = "$jdkBin\jar.exe"
$keytool = "$jdkBin\keytool.exe"

$buildDir = "d:\job-tracker\android\build"
if (Test-Path $buildDir) { Remove-Item $buildDir -Recurse -Force }
New-Item -ItemType Directory -Path "$buildDir\classes" -Force | Out-Null
New-Item -ItemType Directory -Path "$buildDir\dex" -Force | Out-Null

Write-Host "=== 1. Compile Android Resources (aapt2 compile) ==="
& $aapt2 compile --dir "d:\job-tracker\android\res" -o "$buildDir\compiled_res.zip"
if ($LASTEXITCODE -ne 0) { throw "aapt2 compile failed" }

Write-Host "=== 2. Link Android Resources and generate R.java (aapt2 link) ==="
& $aapt2 link -I $platformJar `
    --manifest "d:\job-tracker\android\AndroidManifest.xml" `
    -o "$buildDir\unaligned.apk" `
    --java "d:\job-tracker\android\src" `
    --auto-add-overlay `
    "$buildDir\compiled_res.zip"
if ($LASTEXITCODE -ne 0) { throw "aapt2 link failed" }

Write-Host "=== 3. Compile Java Source Files (javac) ==="
$javaFiles = Get-ChildItem "d:\job-tracker\android\src\com\amara\applytrack\*.java" | ForEach-Object { $_.FullName }
& $javac -encoding UTF-8 -cp $platformJar -d "$buildDir\classes" $javaFiles
if ($LASTEXITCODE -ne 0) { throw "javac failed" }

Write-Host "=== 4. Dex bytecode (d8) ==="
$classFiles = Get-ChildItem "$buildDir\classes\com\amara\applytrack\*.class" | ForEach-Object { $_.FullName }
& $d8 --min-api 26 --output "$buildDir\dex" $classFiles
if ($LASTEXITCODE -ne 0) { throw "d8 failed" }

Write-Host "=== 5. Pack classes.dex into unaligned.apk ==="
Set-Location "$buildDir\dex"
& $jar -uf "$buildDir\unaligned.apk" classes.dex
Set-Location "d:\job-tracker"
if ($LASTEXITCODE -ne 0) { throw "jar uf failed" }

Write-Host "=== 6. Align APK (zipalign) ==="
& $zipalign -v -f -p 4 "$buildDir\unaligned.apk" "$buildDir\aligned.apk"
if ($LASTEXITCODE -ne 0) { throw "zipalign failed" }

Write-Host "=== 7. Ensure Keystore exists ==="
$keystore = "d:\job-tracker\android\applytrack.keystore"
if (-not (Test-Path $keystore)) {
    & $keytool -genkeypair -v `
        -keystore $keystore `
        -alias applytrack `
        -keyalg RSA `
        -keysize 2048 `
        -validity 10000 `
        -storepass applytrack123 `
        -keypass applytrack123 `
        -dname "CN=ApplyTrack, OU=Mobile, O=Amara, L=Bengaluru, ST=Karnataka, C=IN"
}

Write-Host "=== 8. Sign APK (apksigner) ==="
$finalApk = "d:\job-tracker\ApplyTrack.apk"
if (Test-Path $finalApk) { Remove-Item $finalApk -Force }
& $apksigner sign `
    --ks $keystore `
    --ks-pass pass:applytrack123 `
    --ks-key-alias applytrack `
    --key-pass pass:applytrack123 `
    --out $finalApk `
    "$buildDir\aligned.apk"

Write-Host "=== 9. Verify Signature ==="
& $apksigner verify --verbose $finalApk

Write-Host "APK Built Successfully: $finalApk"
Get-Item $finalApk | Select-Object Name, Length, LastWriteTime
