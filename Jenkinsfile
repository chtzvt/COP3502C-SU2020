pipeline {
    agent {
      label 'UCF'
    }

    stages {
        stage('Test') {
            steps {
                sh './build.sh'
            }
        }
    }
}
