pipeline {
  agent { label 'vaskemaskin' }

  environment {
    PROFILE_x86_64 = 'clang-6.0-linux-x86_64'
    PROFILE_x86 = 'clang-6.0-linux-x86'
    CPUS = """${sh(returnStdout: true, script: 'nproc')}"""
    INCLUDEOS_PREFIX = "${env.WORKSPACE}/install"
    CC = 'clang-6.0'
    CXX = 'clang++-6.0'
  }

  stages {
    stage('Setup') {
      steps {
        sh 'rm -rf install || :'
        sh 'mkdir install'
        sh 'cp conan/profiles/* ~/.conan/profiles/'
      }
    }
    stage('Build 64 bit') {
      steps {
        sh 'rm -rf build_x86_64; mkdir build_x86_64'
        sh "cd build_x86_64; cmake -DCONAN_PROFILE=$PROFILE_x86_64 .."
        sh "cd build_x86_64; make -j $CPUS"
        sh 'cd build_x86_64; make install'
      }
    }
    stage('Build 32 bit') {
      steps {
        sh 'rm -rf build_x86; mkdir build_x86'
        sh "cd build_x86; cmake -DCONAN_PROFILE=$PROFILE_x86 -DARCH=i686 -DPLATFORM=x86_nano .."
        sh "cd build_x86; make -j $CPUS"
        sh 'cd build_x86; make install'
      }
    }

    stage('Test') {
      steps {
        echo "No tests so far"
      }
    }

  }
}
