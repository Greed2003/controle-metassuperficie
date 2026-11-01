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
            archiveArtifacts artifacts: "build/libmetasurface_core.a", fingerprint: true
        }
    }
}
