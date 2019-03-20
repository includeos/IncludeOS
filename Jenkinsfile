pipeline {
  agent { label 'vaskemaskin' }

  options {
    checkoutToSubdirectory('src')
  }

  environment {
    CONAN_USER_HOME = "${env.WORKSPACE}"
    REMOTE = "${env.CONAN_REMOTE}"
    PROFILE_x86_64 = 'clang-6.0-linux-x86_64'
    PROFILE_x86 = 'clang-6.0-linux-x86'
    CPUS = """${sh(returnStdout: true, script: 'nproc')}"""
    CC = 'clang-6.0'
    CXX = 'clang++-6.0'
    USER = 'includeos'
    CHAN = 'test'
    COVERAGE_DIR = "${env.COVERAGE_DIR}/${env.JOB_NAME}"
    BINTRAY_CREDS = credentials('devops-includeos-user-pass-bintray')
    SRC = "${env.WORKSPACE}/src"
  }

  stages {
    stage('Setup') {
      steps {
        sh script: "ls -A | grep -v src | xargs rm -r || :", label: "Clean workspace"
        sh script: "conan config install https://github.com/includeos/conan_config.git", label: "conan config install"
      }
    }
    stage('Unit tests') {
      when { changeRequest() }
      steps {
        dir('unittests') {
          sh script: "env CC=gcc CXX=g++ cmake $SRC/test", label: "Cmake"
          sh script: "make -j $CPUS", label: "Make"
          sh script: "ctest", label: "Ctest"
        }
      }
    }
    stage('Build IncludeOS x86') {
      steps {
        build_conan_package("$PROFILE_x86","ON")
      }
    }
    stage('Build IncludeOS x86_64') {
      steps {
        build_conan_package("$PROFILE_x86_64")
      }
    }
    stage('Build chainloader x86') {
      steps {
        build_chainloader_package("$PROFILE_x86")
      }
    }
    stage('Build liveupdate x86_64') {
      steps {
        build_liveupdate_package("$PROFILE_x86_64")
      }
    }
    stage('Integration tests') {
      when { changeRequest() }
      steps {
        dir('integration') {
          sh script: "cmake $SRC/test/integration -DSTRESS=ON, -DCMAKE_BUILD_TYPE=Debug -DCONAN_PROFILE=$PROFILE_x86_64", label: "Cmake"
          sh script: "make -j $CPUS", label: "Make"
          sh script: "ctest -E stress --output-on-failure", label: "Tests"
          sh script: "ctest -R stress -E integration --output-on-failure", label: "Stress test"
        }
      }
    }
    stage('Code coverage') {
      when { changeRequest() }
      steps {
        dir('code_coverage') {
          sh script: "rm -r $COVERAGE_DIR || :", label: "Setup"
          sh script: "env CC=gcc CXX=g++ cmake -DCOVERAGE=ON -DCODECOV_HTMLOUTPUTDIR=$COVERAGE_DIR $SRC/test", label: "Cmake"
          sh script: "make -j $CPUS", label: "Make"
          sh script: "make coverage", label: "Make coverage"
        }
      }
      post {
        success {
           echo "Code coverage: ${env.COVERAGE_ADDRESS}/${env.JOB_NAME}"
        }
      }
    }

    stage('Upload to bintray') {
      when {
        anyOf {
          branch 'master'
          branch 'dev'
        }
      }
      steps {
        script {
          sh script: "conan user -p $BINTRAY_CREDS_PSW -r $REMOTE $BINTRAY_CREDS_USR", label: "Login to bintray"
          def version = sh (
            script: "conan inspect -a version $SRC | cut -d ' ' -f 2",
            returnStdout: true
          ).trim()
          sh script: "conan upload --all -r $REMOTE includeos/${version}@$USER/$CHAN", label: "Upload includeos to bintray"
          sh script: "conan upload --all -r $REMOTE liveupdate/${version}@$USER/$CHAN", label: "Upload liveupdate to bintray"
          sh script: "conan upload --all -r $REMOTE chainloader/${version}@$USER/$CHAN", label: "Upload chainloader to bintray"
        }
      }
    }
  }
}

def build_conan_package(String profile, basic="OFF") {
  sh script: "conan create $SRC $USER/$CHAN -pr ${profile} -o basic=${basic}", label: "Build includeos with profile: $profile"
}

def build_liveupdate_package(String profile) {
  sh script: "conan create $SRC/lib/LiveUpdate $USER/$CHAN -pr ${profile}", label: "Build liveupdate with profile: $profile"
}

def build_chainloader_package(String profile) {
  sh script: "conan create $SRC/src/chainload $USER/$CHAN -pr ${profile}", label: "Build chainloader with profile: $profile"
}
