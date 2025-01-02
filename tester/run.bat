@echo off
echo Building the Docker image...
docker build -t local_tester . || (echo Docker build failed. Exiting. && exit /b 1)

echo Running the Docker container...

 || (echo Docker run failed. Exiting. && exit /b 1)
