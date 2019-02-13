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
        sh 'rm -rf install || : && mkdir install'
        sh 'cp conan/profiles/* ~/.conan/profiles/'
      }
    }
    stage('Build 64 bit') {
      steps {
        sh 'rm -rf build_x86_64 || : && mkdir build_x86_64'
        sh "cd build_x86_64; cmake -DCONAN_PROFILE=$PROFILE_x86_64 .."
        sh "cd build_x86_64; make -j $CPUS"
        sh 'cd build_x86_64; make install'
      }
    }
    stage('Build 32 bit') {
      steps {
        sh 'rm -rf build_x86 || : && mkdir build_x86'
        sh "cd build_x86; cmake -DCONAN_PROFILE=$PROFILE_x86 -DARCH=i686 -DPLATFORM=x86_nano .."
        sh "cd build_x86; make -j $CPUS"
        sh 'cd build_x86; make install'
      }
    }
    stage('Unit tests') {
      steps {
        sh 'rm -rf unittests || : && mkdir unittests'
        sh 'cd unittests; cmake ../test'
        sh "cd unittests; make -j $CPUS"
        sh 'cd unittests; ctest'
      }
    }
    stage('Code coverage') {
      steps {
        sh 'rm -rf coverage || : && mkdir coverage'
        sh 'cd coverage; env CC=gcc CXX=g++ cmake -DCOVERAGE=ON ../test'
        sh "cd coverage; env CC=gcc CXX=g++ make -j $CPUS"
      }
    }
    stage('Integration tests') {
      sh 'rm -rf integration || : && mkdir integration'
      sh 'cd integration; cmake ../test/integration'
      sh "cd integration; make -j $CPUS"
      // TODO: Run the integration tests
      // sh 'cd integration; ctest'
    }
  }
}
