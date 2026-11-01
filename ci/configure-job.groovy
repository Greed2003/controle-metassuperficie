import hudson.plugins.git.BranchSpec
import hudson.plugins.git.GitSCM
import jenkins.model.Jenkins
import org.jenkinsci.plugins.workflow.cps.CpsScmFlowDefinition
import org.jenkinsci.plugins.workflow.job.WorkflowJob

def controller = Jenkins.get()
def jobName = "controle-metassuperficie"
def repository = "https://github.com/Greed2003/controle-metassuperficie.git"
def branch = args.length > 0 ? args[0] : "main"

def scm = new GitSCM(
    GitSCM.createRepoList(repository, null),
    [new BranchSpec("*/" + branch)],
    null,
    null,
    []
)

def definition = new CpsScmFlowDefinition(scm, "Jenkinsfile")
definition.setLightweight(true)

def job = controller.getItem(jobName)

if (job != null && !(job instanceof WorkflowJob)) {
    throw new IllegalStateException("Ja existe outro tipo de job com esse nome.")
}

if (job == null) {
    job = controller.createProject(WorkflowJob.class, jobName)
}

job.setDefinition(definition)
job.save()

println "Job ${jobName} configurado para a branch ${branch}."
