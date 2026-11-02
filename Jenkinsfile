pipeline {
    agent { label "metasurface-cpp" }

    options {
        skipDefaultCheckout(true)
        disableConcurrentBuilds()
        buildDiscarder(logRotator(numToKeepStr: "20"))
        timeout(time: 10, unit: "MINUTES")
    }

    triggers {
        pollSCM("H/5 * * * *")
    }

    stages {
        stage("Checkout") {
            steps {
                deleteDir()
                checkout scm
            }
        }

        stage("Configurar") {
            steps {
                sh "cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug -DBUILD_TESTING=ON"
            }
        }

        stage("Compilar") {
            steps {
                sh "cmake --build build --parallel 2"
            }
        }

        stage('Compilar AVR') {
            steps {
                sh 'avr-g++ --version'
                sh 'cmake -S . -B build/avr -DCMAKE_TOOLCHAIN_FILE=cmake/avr-mega2560.cmake -DCMAKE_BUILD_TYPE=MinSizeRel -DBUILD_TESTING=OFF'
                sh 'cmake --build build/avr --parallel 2'
                sh 'avr-objdump -f build/avr/libmetasurface_core.a'
                sh 'cmake "-DARCHIVE=build/avr/libmetasurface_core.a" -P ci/check-avr-catalog.cmake'
            }
        }

        stage("Testar") {
            steps {
                dir("build") {
                    sh "ctest --output-on-failure --no-tests=error --output-junit test-results.xml"
                }
            }
            post {
                always {
                    junit testResults: "build/test-results.xml", allowEmptyResults: false
                }
            }
        }
    }

    post {
        success {
            archiveArtifacts artifacts: "build/libmetasurface_core.a,build/avr/libmetasurface_core.a", fingerprint: true
        }
    }
}
