#define _POSIX_SOURCE

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <string.h>
#include <sys/types.h>
#include <signal.h>

#include "readcmd.h"
#include "job_list.h"

#define E_CHDIR 1

// ==============================
// === GESTIONNAIRE D'ERREURS ===
// ==============================
// Parametres:	ret			code de retour
//				code		erreur associee
int ok_cmd (int ret, int code) {
	if (ret == -1) {
		switch (code) {
			case E_CHDIR:
				perror ("Erreur: chdir\n");
		}
		exit (code);
	}
	return ret;
}


// ==========================
// === COMMANDES INTERNES ===
// ==========================

// === exit ===
// Sort du minishell
int cmd_exit () {
	exit (0);
	return 0;
}

// === cd ===
// Affiche les fichiers et dossiers du repertoire courant
int cmd_cd (struct cmdline *cmd) {
	int ret_chdir = 0;
	if (cmd->seq[0][1] != NULL) {
		// Si un chemin est specifie, on s'y rend
		ret_chdir = chdir (cmd->seq[0][1]);
		ok_cmd (ret_chdir, E_CHDIR);
	} else {
		// Sinon, on va au /home
		ret_chdir = chdir (getenv("HOME"));
		ok_cmd (ret_chdir, E_CHDIR);
	}
	return 0;
}

// === lj ===
// Donne la liste des processus lances depuis le minishell
int cmd_lj () {
	display_job_list ();
	return 0;
}

// === sj ===
// Suspend un processus
// Parametre:	id		id du processus a suspendre
int cmd_sj (int id) {
	// On récupère le pid du processus correspond à cet id
	pid_t pid = get_pid_from_id (id);
	if (pid != -1) {
		// On suspend le processus
		kill (pid, SIGSTOP);
	} else {
		printf ("sj: %d: ce processus n'existe pas\n", id);
	}
	return 0;
}

// === bg ===
// Reprend en arriere-plan un processus suspendu
// Parametre:	id		id du processus a restaurer
int cmd_bg (int id) {
	// On recupere le pid du processus correspond à cet id
	pid_t pid = get_pid_from_id (id);
	if (pid != -1) {
		// On restaure le processus
		kill (pid, SIGCONT);
	} else {
		printf ("bg: %d: ce processus n'existe pas\n", id);
	}
	return 0;
}

// === fg ===
// Reprend en avant-plan un processus suspendu ou en arriere-plan
// Parametre:	id		id du processus a restaurer
int cmd_fg (int id) {
	// On recupère le pid du processus correspond a cet id
	pid_t pid = get_pid_from_id (id);
	if (pid != -1) {
		// On restaure le processus
		kill (pid, SIGCONT);
		
		// Avant-plan
		int status;
		int ret_waitpid = waitpid (pid, &status, WUNTRACED);
		if (ret_waitpid > 0 && WIFEXITED (status)) {
			remove_job (pid);
		}
	} else {
		printf ("fg: %d: ce processus n'existe pas\n", id);
	}
	return 0;
}

// === susp ===
// Suspend l'execution du minishell
int cmd_susp () {
	raise (SIGSTOP);

	return 0;
}

// ==============================
// === EXECUTEUR DE COMMANDES ===
// ==============================

// Execute une commande interne si elle existe
int exec_cmd_interne (struct cmdline *cmd) {
	// Si on execute une cmd interne, exec=0. Sinon, exec=-1
	int exec = -1;
	// Le nom de la commande
	char* cmd_name = cmd->seq[0][0];

	// Execute la fonction correspond a la commande
	if (strcmp (cmd_name, "exit") == 0) {
		exec = cmd_exit ();
	} else if (strcmp (cmd_name, "cd") == 0) {
		exec = cmd_cd (cmd);
	} else if (strcmp (cmd_name, "lj") == 0) {
		exec = cmd_lj ();
	} else if (strcmp (cmd_name, "sj") == 0) {
		int id = atoi(cmd->seq[0][1]);
		exec = cmd_sj (id);
	} else if (strcmp (cmd_name, "bg") == 0) {
		int id = atoi(cmd->seq[0][1]);
		exec = cmd_bg (id);
	} else if (strcmp (cmd_name, "fg") == 0) {
		int id = atoi(cmd->seq[0][1]);
		exec = cmd_fg (id);
	} else if (strcmp (cmd_name, "susp") == 0) {
		exec = cmd_susp ();
	}

	return exec;
}







