pipeline {
  agent { label 'vaskemaskin' }
  environment {
    PROFILE = 'clang-6.0-linux-x86_64'
    CPUS = """${sh(returnStdout: true, script: 'nproc')}"""
    INCLUDEOS_PREFIX = "${env.WORKSPACE}/install"
    CC = 'clang-6.0'
    CXX = 'clang++-6.0'
  }

  stages {
    stage('Build') {
      steps {
        sh 'mkdir -p build; rm -r build/*'
        sh 'cp conan/profiles/* ~/.conan/profiles/'
        sh "cd build; cmake -DCONAN_PROFILE=$PROFILE .."
        sh "cd build; make -j $CPUS"
        sh "cd build; make install"
      }
    }

    stage('Test') {
      steps {
        echo "Testing"
      }
    }

  }
}
