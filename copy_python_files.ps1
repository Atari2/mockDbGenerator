$resourceDir = "$pwd\mockDbGeneratorUI\mockDbGeneratorUI\resources"
Copy-Item -Path "mockDbGenerator.py" -Destination "$resourceDir\mockDbGenerator.py" -Force
Copy-Item -Path "typeWrappers" -Destination "$resourceDir\typeWrappers" -Force -Recurse
Copy-Item -Path "dataGenerators" -Destination "$resourceDir\dataGenerators" -Force -Recurse
Copy-Item -Path "structureReader" -Destination "$resourceDir\structureReader" -Force -Recurse