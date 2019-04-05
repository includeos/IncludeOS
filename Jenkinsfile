pipeline {
  agent { label 'ubuntu-18.04' }
  options { checkoutToSubdirectory('src') }
  environment {
    CONAN_USER_HOME = "${env.WORKSPACE}"
    REMOTE = "${env.CONAN_REMOTE}"
    PROFILE_x86_64 = 'clang-6.0-linux-x86_64'
    PROFILE_x86 = 'clang-6.0-linux-x86'
    CPUS = """${sh(returnStdout: true, script: 'nproc')}"""
    USER = 'includeos'
    CHAN_LATEST = 'latest'
    CHAN_STABLE = 'stable'
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
          sh script: "conan install $SRC/test -pr $PROFILE_x86_64", label: "Conan install"
          sh script: ". ./activate.sh; cmake $SRC/test -DCMAKE_BUILD_TYPE=Debug", label: "Cmake"
          sh script: "make -j $CPUS", label: "Make"
          sh script: "ctest", label: "Ctest"
        }
      }
    }
    stage('Build IncludeOS x86') {
      steps {
        build_conan_package("$SRC", "$USER/$CHAN_LATEST", "$PROFILE_x86", "nano")
        script { VERSION = sh(script: "conan inspect -a version $SRC | cut -d ' ' -f 2", returnStdout: true).trim() }
      }
    }
    stage('Build IncludeOS x86_64') {
      steps {
        build_conan_package("$SRC", "$USER/$CHAN_LATEST", "$PROFILE_x86_64")
      }
    }
    stage('Build chainloader x86') {
      steps {
        build_conan_package("$SRC/src/chainload", "$USER/$CHAN_LATEST", "$PROFILE_x86")
      }
    }
    stage('Build liveupdate x86_64') {
      steps {
        build_conan_package("$SRC/lib/LiveUpdate", "$USER/$CHAN_LATEST", "$PROFILE_x86_64")
      }
    }
    stage('Integration tests') {
      when { changeRequest() }
      steps {
        dir('integration') {
          sh script: "conan install $SRC/test/integration -pr $PROFILE_x86_64", label: "Conan install"
          sh script: ". ./activate.sh; cmake $SRC/test/integration -DSTRESS=ON -DCMAKE_BUILD_TYPE=Debug", label: "Cmake"
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
      parallel {
        stage('Latest release') {
          when {
            anyOf {
              branch 'master'
              branch 'dev'
              buildingTag()
            }
          }
          steps {
            upload_package("includeos", "$CHAN_LATEST")
            upload_package("liveupdate", "$CHAN_LATEST")
            upload_package("chainloader", "$CHAN_LATEST")
          }
        }
        stage('Stable release') {
          when { buildingTag() }
          steps {
            sh script: "conan copy --all includeos/$VERSION@$USER/$CHAN_LATEST $USER/$CHAN_STABLE", label: "Copy includeos to stable channel"
            upload_package("includeos", "$CHAN_STABLE")
            sh script: "conan copy --all liveupdate/$VERSION@$USER/$CHAN_LATEST $USER/$CHAN_STABLE", label: "Copy liveupdate to stable channel"
            upload_package("liveupdate", "$CHAN_STABLE")
            sh script: "conan copy --all chainloader/$VERSION@$USER/$CHAN_LATEST $USER/$CHAN_STABLE", label: "Copy chainiloader to stable channel"
            upload_package("chainloader", "$CHAN_STABLE")
          }
        }
      }
    }
  }
}

def build_conan_package(String src, user_chan, profile, platform="") {
  sh script: "platform=${platform}; conan create $src $user_chan -pr ${profile} \${platform:+-o platform=$platform}", label: "Build with profile: $profile"
}

def upload_package(String package_name, channel) {
  sh script: """
    conan user -p $BINTRAY_CREDS_PSW -r $REMOTE $BINTRAY_CREDS_USR
    conan upload --all -r $REMOTE $package_name/$VERSION@$USER/$channel
  """, label: "Upload $package_name to bintray channel: $channel"
}
