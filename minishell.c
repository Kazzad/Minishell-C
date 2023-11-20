#define _POSIX_SOURCE

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <string.h>
#include <sys/types.h>
#include <signal.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/stat.h>

#include "readcmd.h"
#include "job_list.h"
#include "cmd_internes.h"

#define E_WAITPID 1
#define E_FORK 2
#define E_EXECVP 3
#define E_SIGACTION 4
#define E_OPEN 5
#define E_DUP2 6
#define E_CLOSE 7
#define E_PIPE 8
#define E_SPM 9

// ID du processus en avant plan
int id_fg = -1;

// ==============================
// === GESTIONNAIRE D'ERREURS ===
// ==============================
// Parametres:	ret			code de retour
//				code		erreur associee
int ok (int ret, int code) {
	if (ret == -1) {
		switch (code) {
			case E_WAITPID:
				perror ("Erreur: waitpid\n");
				break;
			case E_FORK:
				perror ("Erreur: fork\n");
				break;
			case E_EXECVP:
				perror ("Erreur: execvp\n");
				break;
			case E_SIGACTION:
				perror ("Erreur: sigaction\n");
				break;
			case E_OPEN:
				perror ("Erreur: open\n");
				break;
			case E_DUP2:
				perror ("Erreur: dup2\n");
				break;
			case E_CLOSE:
				perror ("Erreur: close\n");
				break;
			case E_PIPE:
				perror ("Erreur: pipe\n");
				break;
			case E_SPM:
				perror ("Erreur: sigprocmask\n");
				break;
		}
		exit (code);
	}

	return ret;
}

// ==============
// === PROMPT ===
// ==============
void display_prompt () {
	printf (">>> ");
}

// ================
// === HANDLERS ===
// ================
// Handler pour SIGCHLD
void handler_chld () {
	pid_t pid;
	int child_status;
	do {
		pid = waitpid (-1, &child_status, WNOHANG | WUNTRACED | WCONTINUED);
		
		if (pid < 1) {
			continue;
		}
				
		if (WIFEXITED (child_status)) {
			// Si terminaison normale, retire le job
			remove_job (pid);
		}

		if (WIFSIGNALED (child_status)) {
			// Si Ctrl-C ou SIGINT, retire le job
			remove_job (pid);
		}
		
		if (WIFSTOPPED (child_status)) {
			// Si Ctrl-Z ou SIGTSTP, change le statut du job
			int id = get_id_from_pid (pid);
			if (id > 0) {
				change_job_status (id, (status)Suspendu);
			}
		}

		if (WIFCONTINUED (child_status)) {
			// Sig SIGCONT, change le statut du job
			int id = get_id_from_pid (pid);
			if (id > 0) {
				change_job_status (id, (status)Actif);
				id_fg = id;
			}
		}

	} while (pid > 0);
}
// Handler pour SIGTSTP
void handler_tstp () {
	pid_t pid = get_pid_from_id (id_fg);
	if (pid > 0) {
		// Suspension du processus
		kill (pid, SIGSTOP);
	} else {
		// Reaffiche le prompt avec un Ctrl-Z echoue
		printf ("\n");
		display_prompt ();
	}
}
// Handler pour SIGINT
void handler_int () {
	pid_t pid = get_pid_from_id (id_fg);
	if (pid > 0) {
		// Terminaison du processus
		kill (pid, SIGTERM);
	} else {
		// Reaffiche le prompt avec un Ctrl-C echoue
		printf ("\n");
		display_prompt ();
	}
}

// ===================
// === REDIRECTION ===
// ===================
// Redirige l'entree et/ou la sortie standard d'une commande vers un fichier
// Parametres:	in			nom du fichier vers lequel l'entrée doit etre redirigee
//				out			nom du fichier vers lequel la sortie doit etre redirigee
void redirection (char *in, char *out) {
	// Configuration (eventuelle) du fichier vers lequel l'entree doit être redirigee
	if (in != NULL) {
		// Ouverture du fichier (lecture)
		int fd_in = open (in, O_RDONLY, 0644);
		ok (fd_in, E_OPEN);
		// Copie du descripteur sur l'entrée standard
		int fd = dup2 (fd_in, 0);
		ok (fd, E_DUP2);
		// Fermeture du descripteur initial
		int ret_close = close (fd_in);
		ok (ret_close, E_CLOSE);
	}
				
	// Configuration (eventuelle) du fichier vers lequel la sortie doit être redirigee
	if (out != NULL) {
		// Ouverture du fichier (écriture)
		int fd_out = open (out, O_WRONLY | O_CREAT, 0644);
		ok(fd_out, E_OPEN);
		// Copie du descripteur sur la sortie standard
		int fd = dup2 (fd_out, 1);
		ok (fd, E_DUP2);
		// Fermeture du descripteur initial
		int ret_close = close (fd_out);
		ok (ret_close, E_CLOSE);
	}
}

// ================
// === MASQUAGE ===
// ================
// Masque un signal
// Parametre:	signum		signal a masquer
void mask_signal (int signum) {
	// Construction du set contenant signum
	sigset_t mask_set;
	sigemptyset (&mask_set);
	sigaddset (&mask_set, signum);
	// Masquage du set
	int ret_spm = sigprocmask (SIG_BLOCK, &mask_set, NULL);
	ok (ret_spm, E_SPM);
}

// ================
// === HANDLING ===
// ================
// Redirige SIGCHLD, SIGTSTP et SIGINT vers des handlers
void handle () {
	// Redirige les signaux SIGCHLD vers la fonction handler_chld
	struct sigaction sa_chld;
	memset (&sa_chld, 0, sizeof(sa_chld));
	sa_chld.sa_handler = handler_chld;
	sigemptyset (&sa_chld.sa_mask);
	sa_chld.sa_flags = SA_RESTART;
	int ret_sigaction = sigaction(SIGCHLD, &sa_chld, NULL);
	ok (ret_sigaction, E_SIGACTION);

	// Redirige les signaux SIGTSTP vers la fonction handler_tstp
	struct sigaction sa_tstp;
	memset (&sa_tstp, 0, sizeof(sa_tstp));
	sa_tstp.sa_handler = handler_tstp;
	sigemptyset (&sa_tstp.sa_mask);
	ret_sigaction = sigaction(SIGTSTP, &sa_tstp, NULL);
	ok (ret_sigaction, E_SIGACTION);
	
	// Redirige les signaux SIGINT vers la fonction handler_int
	struct sigaction sa_int;
	memset (&sa_int, 0, sizeof(sa_int));
	sa_int.sa_handler = handler_int;
	sigemptyset (&sa_int.sa_mask);
	ret_sigaction = sigaction(SIGINT, &sa_int, NULL);
	ok (ret_sigaction, E_SIGACTION);
}

// ============
// === MAIN ===
// ============
int main () {
	// Ligne de commande (donnee par readcmd)
	struct cmdline *cmd;

	// Liste de l'id courant: l'id attribue au prochain processus cree
	int job_id = 1;

	// Redirection des signaux SIGCHLD, SIGTSTP, SIGINT vers des handlers
	handle ();

	// Boucle infinie d'execution du minishell
	display_prompt ();
	while (1) {
		// === SAISIE DE LA LIGNE DE COMMANDE ===

		// Affichage du prompt
		if (cmd != NULL) {
			display_prompt ();
		}
		
		// Lecture de la commande de l'utilisateur
		cmd = readcmd ();

		// Gestion du cas d'une cmd vide
		if (cmd == NULL || cmd->seq[0] == NULL) {
			continue;
		}

		// Gestion du cas d'une erreur de cmd
		if (cmd->err != NULL) {
			printf ("--- Erreur de structure de la commande : %s\n", cmd->err);
			continue;
		}

		// === TRAITEMENT DE LA LIGNE DE COMMANDE ===

		// Si la ligne saisie est une cmd interne, l'execute sans processus fils
		int cmd_interne = exec_cmd_interne (cmd);

		// Sinon, execute la ligne de cmd avec un processus fils
		if (cmd_interne == -1) {
			// Une ligne de commande peut contenir plusieurs commandes
			// Une commande correspond a 1 processus fils
			// Dans le cas d'un pipeline, les processus sont relies par des tubes
			/* Exemple de "cat f1 f2 | grep int | wc -l" :
				 minishell
				 |___________________
				 |    ->  |     ->  |
				 cat o==o grep o==o wc
			*/

			int prev[2];	// Tube precedant le processus fils actuel
			int next[2];	// Tube precede par le processus fils actuel

			// Boucle sur toutes les commandes de la ligne
			int i = 0;
			while (cmd->seq[i] != NULL) {
				// === TRAITEMENT D'UNE COMMANDE ===

				int premier = (i == 0);			// Premiere commande de la ligne ?	
				int dernier = (cmd->seq[i+1] == NULL);	// Derniere commande de la ligne ?
				
				// Creation du pipe vers le prochain processus
				if (!dernier) {
					int ret_pipe = pipe (next);
					ok (ret_pipe, E_PIPE);
				}
		
				// Creation du processus fils
				int pid_fils = fork ();
				ok (pid_fils, E_FORK);
				
				int ret_close;
				int ret_dup2;
				// FILS : execution de la commande
				if (pid_fils == 0) {
					// === GESTION DU PIPELINE ===

					// Lecture de la sortie du tube precedant
					if (!premier) {
						ret_close = close (prev[1]);
						ok (ret_close, E_CLOSE);
				
						ret_dup2 = dup2 (prev[0], 0);
						ok (ret_dup2, E_DUP2);
					}
			
					// Ecriture dans l'entree du tube suivant
					if (!dernier) {
						ret_close = close (next[0]);
						ok (ret_close, E_CLOSE);
			
						ret_dup2 = dup2 (next[1], 1);
						ok (ret_dup2, E_DUP2);
					}
					
					// Fermeture des tubes
					if (!dernier) {
						ret_close = close (next[1]);
						ok (ret_close, E_CLOSE);
					}
					if (!premier) {
						ret_close = close (prev[0]);
						ok (ret_close, E_CLOSE);
					}
			
					// === MASQUAGE ===
					mask_signal (SIGINT);	// Pour eviter la diffusion au fils apres Ctrl-C
					mask_signal (SIGTSTP);	// Pour eviter la diffusion au fils apres Ctrl-Z
				
					// === REDIRECTION ===
					// Associe l'entree/la sortie standard à un fichier
					redirection (cmd->in, cmd->out);

					// === EXECUTION ===
					int ret_execvp = execvp (cmd->seq[i][0], cmd->seq[i]);
					if (ret_execvp == -1 && errno == 2) { // Commande inconue
						printf ("minishell: %s: commande introuvable\n", cmd->seq[i][0]);
					} else {
						ok (ret_execvp, E_EXECVP);
					}
					

					// === TERMINAISON ===
					exit(1);
				}
				// PERE : mise en attente
				else {
					// === GESTION DU PIPELINE ===

					// Fermeture des tubes
					if (!premier) {
						ret_close = close (prev[0]);
						ok (ret_close, E_CLOSE);
				
						ret_close = close (prev[1]);
						ok (ret_close, E_CLOSE);
					}
			
					// Le tube suivant devient le tube precedant
					prev[0] = next[0];
					prev[1] = next[1];
				
					// === GESTION DE LA TABLE DES PROCESSUS ===

					// Si aucun processus en cours, l'ID courant revient a 1
					if (is_job_list_empty ()) {
						job_id = 1;
					}

					// Ce processus est en avant plan
					id_fg = job_id;

					// Enregistrement du processus
					add_job (job_id, pid_fils, Actif, cmd->seq[0][0]);
					job_id++;

					// === PROCESSUS EN AVANT PLAN ===

					// Sans '&', processus mis en avant plan
					if (cmd->backgrounded == NULL) {
						// Attend un changement d'etat du processus
						int child_status;
						int ret_waitpid = waitpid (pid_fils, &child_status, WUNTRACED);
					

						if (ret_waitpid == -1) {
							printf("\n");
						} else if (WIFEXITED (child_status)) {
							// Si le processus termine normalement, retire le job
							remove_job (pid_fils);
						} else if (WIFSTOPPED (child_status)) {
							// Si le processus est stoppé (hors Ctrl-Z), change le statut du job
							int id = get_id_from_pid (pid_fils);
							if (id > 0) {
								change_job_status (id, (status)Suspendu);
							}
						}
					}
	 
					// Il n'y a plus de processus en avant plan
					id_fg = -1;
				}
				
				// On passe a la commande suivante dans la ligne
				i++;
			}
		}
	}

	return EXIT_SUCCESS;
}

