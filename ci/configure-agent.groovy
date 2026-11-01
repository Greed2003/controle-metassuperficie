import hudson.model.Node
import hudson.slaves.DumbSlave
import hudson.slaves.JNLPLauncher
import jenkins.model.Jenkins

def controller = Jenkins.get()
def agentName = "metasurface-cpp"
def agent = controller.getNode(agentName)
def isNew = agent == null

if (isNew) {
    agent = new DumbSlave(agentName, "/home/jenkins/agent", new JNLPLauncher())
}

agent.setNumExecutors(1)
agent.setMode(Node.Mode.EXCLUSIVE)
agent.setLabelString("metasurface-cpp")

if (isNew) {
    controller.addNode(agent)
} else {
    controller.updateNode(agent)
}

controller.setNumExecutors(0)
controller.save()

def secretFile = new File(controller.rootDir, "secrets/metasurface-agent")
secretFile.text = agent.toComputer().getJnlpMac()

println "Agente metasurface-cpp cadastrado."
