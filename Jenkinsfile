pipeline {
  agent { label 'vaskemaskin' }

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
  }

  stages {
    stage('Setup') {
      steps {
        sh script: "conan config install https://github.com/includeos/conan_config.git", label: "conan config install"
      }
    }

    stage('Unit tests') {
      steps {
        sh script: "mkdir -p unittests", label: "Setup"
        sh script: "cd unittests; env CC=gcc CXX=g++ cmake ../test", label: "Cmake"
        sh script: "cd unittests; make -j $CPUS", label: "Make"
        sh script: "cd unittests; ctest", label: "Ctest"
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
      steps {
        sh script: "mkdir -p integration", label: "Setup"
        sh script: "cd integration; cmake ../test/integration -DSTRESS=ON, -DCMAKE_BUILD_TYPE=Debug -DCONAN_PROFILE=$PROFILE_x86_64", label: "Cmake"
        sh script: "cd integration; make -j $CPUS", label: "Make"
        sh script: "cd integration; ctest -E stress --output-on-failure", label: "Tests"
        sh script: "cd integration; ctest -R stress -E integration --output-on-failure", label: "Stress test"
      }
    }
    stage('Code coverage') {
      steps {
        sh script: "mkdir -p coverage; rm -r $COVERAGE_DIR || :", label: "Setup"
        sh script: "cd coverage; env CC=gcc CXX=g++ cmake -DCOVERAGE=ON -DCODECOV_HTMLOUTPUTDIR=$COVERAGE_DIR ../test", label: "Cmake"
        sh script: "cd coverage; make -j $CPUS", label: "Make"
        sh script: "cd coverage; make coverage", label: "Make coverage"
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
          def version = sh (
            script: 'conan inspect -a version . | cut -d " " -f 2',
            returnStdout: true
          ).trim()
          sh script: "conan upload --all -r $REMOTE includeos/${version}@$USER/$CHAN", label: "Upload includeos to bintray"
          sh script: "conan upload --all -r $REMOTE liveupdate/${version}@$USER/$CHAN", label: "Upload liveupdate to bintray"
          sh script: "conan upload --all -r $REMOTE chainloader/${version}@$USER/$CHAN", label: "Upload chainloader to bintray"
        }
      }
    }
  }
  post {
    cleanup {
      sh script: """
        VERSION=\$(conan inspect -a version lib/LiveUpdate | cut -d " " -f 2)
        conan remove liveupdate/\$VERSION@$USER/$CHAN -f || echo 'Could not remove. This does not fail the pipeline'
      """, label: "Cleaning up and removing conan package"
      sh script: """
        VERSION=\$(conan inspect -a version src/chainload | cut -d " " -f 2)
        conan remove chainloader/\$VERSION@$USER/$CHAN -f || echo 'Could not remove. This does not fail the pipeline'
      """, label: "Cleaning up and removing conan package"
      sh script: """
        VERSION=\$(conan inspect -a version . | cut -d " " -f 2)
        conan remove includeos/\$VERSION@$USER/$CHAN -f || echo 'Could not remove. This does not fail the pipeline'
      """, label: "Cleaning up and removing conan package"
    }
  }
}

def build_conan_package(String profile, basic="OFF") {
  sh script: "conan create . $USER/$CHAN -pr ${profile} -o basic=${basic}", label: "Build includeos with profile: $profile"
}

def build_liveupdate_package(String profile) {
  sh script: "conan create lib/LiveUpdate $USER/$CHAN -pr ${profile}", label: "Build liveupdate with profile: $profile"
}

def build_chainloader_package(String profile) {
  sh script: "conan create src/chainload $USER/$CHAN -pr ${profile}", label: "Build chainloader with profile: $profile"
}
