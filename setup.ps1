git clone https://github.com/microsoft/vcpkg
cd .\vcpkg
.\bootstrap-vcpkg.bat
cd ..
.\vcpkg\vcpkg.exe install --triplet=x64-windows
.\vcpkg\vcpkg.exe integrate install