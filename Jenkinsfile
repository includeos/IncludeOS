pipeline {
  agent { label 'vaskemaskin' }

  environment {
    PROFILE_x86_64 = 'clang-6.0-linux-x86_64'
    PROFILE_x86 = 'clang-6.0-linux-x86'
    CPUS = """${sh(returnStdout: true, script: 'nproc')}"""
    INCLUDEOS_PREFIX = "${env.WORKSPACE}/install"
    CC = 'clang-6.0'
    CXX = 'clang++-6.0'
    USER = 'includeos'
    CHAN = 'test'
    MOD_VER= '0.13.0'
  }

  stages {
    stage('Setup') {
      steps {
        sh 'mkdir -p install'
        sh 'cp conan/profiles/* ~/.conan/profiles/'
      }
    }
    stage('Unit tests') {
      steps {
        sh 'mkdir -p unittests'
        sh 'cd unittests; env CC=gcc CXX=g++ cmake ../test'
        sh "cd unittests; make -j $CPUS"
        sh 'cd unittests; ctest'
      }
    }
    stage('liveupdate x86_64') {
      steps {
      	build_editable('lib/LiveUpdate','liveupdate')
      }
    }
    stage('mana x86_64') {
      steps {
      	build_editable('lib/mana','mana')
      }
    }
    stage('mender x86_64') {
      steps {
      	build_editable('lib/mender','mender')
      }
    }
    stage('uplink x86_64') {
      steps {
      	build_editable('lib/uplink','uplink')
      }
    }
    stage('microLB x86_64') {
      steps {
      	build_editable('lib/microLB','microlb')
      }
    }
    stage('Build 32 bit') {
      steps {
        sh 'mkdir -p build_x86'
        sh "cd build_x86; cmake -DCONAN_PROFILE=$PROFILE_x86 -DARCH=i686 -DPLATFORM=x86_nano .."
        sh "cd build_x86; make -j $CPUS"
        sh 'cd build_x86; make install'
      }
    }
    /* TODO
    stage('build chainloader 32bit') {
      steps {
  	sh """
          cd src/chainload
    	  rm -rf build || :&& mkdir build
    	  cd build
    	  conan link .. chainloader/$MOD_VER@$USER/$CHAN --layout=../layout.txt
     	  conan install .. -pr $PROFILE_x86 -u
    	  cmake --build . --config Release
  	"""
      }
    }
    */
    stage('Build 64 bit') {
      steps {
        sh 'mkdir -p build_x86_64'
        sh "cd build_x86_64; cmake -DCONAN_PROFILE=$PROFILE_x86_64 .."
        sh "cd build_x86_64; make -j $CPUS"
        sh 'cd build_x86_64; make install'
      }
    }
    stage('Code coverage') {
      steps {
        sh 'mkdir -p coverage'
        sh 'cd coverage; env CC=gcc CXX=g++ cmake -DCOVERAGE=ON ../test'
        sh "cd coverage; make -j $CPUS"
        sh 'cd coverage; make coverage'
      }
    }
    stage('Integration tests') {
      steps {
        sh 'mkdir -p integration'
        sh 'cd integration; cmake ../test/integration -DSTRESS=ON, -DCMAKE_BUILD_TYPE=Debug'
        sh "cd integration; make -j $CPUS"
        sh 'cd integration; ctest -E stress --output-on-failure'
        sh 'cd integration; ctest -R stress -E integration --output-on-failure'
      }
    }
  }
}

def build_editable(String location, String name) {
  sh """
    cd $location
    mkdir -p build
    cd build
    conan link .. $name/$MOD_VER@$USER/$CHAN --layout=../layout.txt
    conan install .. -pr $PROFILE_x86_64 -u
    cmake -DARCH=x86_64 ..
    cmake --build . --config Release
  """
}
