#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <ctype.h>
#include <assert.h>

#include "linked_list.h"
#include "node.h"

int is_operator(char text) {
	if (text == '<' || text == '>' || text == '|' || text == '&'
			|| text == ';') {
		return 1;
	}
	return 0;
}

int is_double_operator(char text) {
	if (text == '|' || text == '&') {
		return 1;
	}
	return 0;
}

char*
read_term(char *text, int ii) {
	int nn = 0;
	while ((ii + nn) < strlen(text)
			&& !(is_operator(text[ii + nn]) || isspace(text[ii + nn]))) {
		nn = nn + 1;
	}
	char *term = malloc(nn + 1);
	strncpy(term, text + ii, nn);
	term[nn] = '\0';
	return term;
}

char*
read_parens(char *text, int ii) {
	int nn = 1;
	int deep = 1;
	int quoteCounter = 0;
	while (1) {
		if (text[nn + ii] == '"') {
			quoteCounter = 1 - quoteCounter;
		} else if (text[nn + ii] == '(') {
			deep = deep + 1;
		} else if ((quoteCounter == 0) && (text[nn + ii] == ')')) {
			deep = deep - 1;
		}
		nn = nn + 1;
		if (deep < 1) {
			break;
		}
	}
	char *term = malloc(nn + 1);
	strncpy(term, text + ii, nn);
	term[nn] = '\0';
	return term;
}

char*
read_quote(char *text, int ii) {
	int nn = 1;
	int deep = 1;
	while (1) {
		if (text[nn + ii] == '"') {
			deep = 0;
		}
		nn = nn + 1;
		if (deep < 1) {
			break;
		}
	}
	char *term = malloc(nn+1);
	strncpy(term, text + ii, nn);
	term[nn] = '\0';
	return term;
}


linked_list*
tokenize(char *text) {
	linked_list *tokens = 0;

	int nn = strlen(text);
	int ii = 0;
	while (ii < nn) {
		if (ii < nn - 1) {
			if (is_double_operator(text[ii]) && text[ii] == text[ii + 1]) {
				char op[4] = "x";
				op[0] = text[ii];
				op[1] = text[ii + 1];
				tokens = cons(op, tokens);
				ii = ii + 2;
				continue;
			}
		}
		if (isspace(text[ii])) {
			ii = ii + 1;
		} else if (is_operator(text[ii])) {
			char op[4] = "x";
			op[0] = text[ii];
			tokens = cons(op, tokens);
			ii = ii + 1;
		} else if (text[ii] == '(') {
			char *term = read_parens(text, ii);
			tokens = cons(term, tokens);
			ii = ii + strlen(term);
			free(term);
		} else if (text[ii] == '"') {
			char *term = read_quote(text, ii);
			tokens = cons(term, tokens);
			ii = ii + strlen(term);
			free(term);
		} else {
			char *term = read_term(text, ii);
			tokens = cons(term, tokens);
			ii = ii + strlen(term);
			free(term);
		}
	}
	return tokens;
}

void liner(char *cmd);

int execute(node *nodes) {
	int cpid;
	int code;
	if (strcmp(nodes->op, "=") == 0) {
		if (strcmp(nodes->command, "exit") == 0) {
			exit(0);
		} else {
			char *cmd = nodes->command;
			char com_args[256];
			char com_cmd[256];
			if (cmd[strlen(cmd) - 1] == '\n') {
				cmd[strlen(cmd) - 1] = '\0';
			}
			int assigner = 0;
			for (int ii = 0; ii < strlen(cmd); ++ii) {
				if (cmd[ii] == '=') {
					assigner = ii;
				}
			}
			if (assigner != 0) { // assigns vars to their own file LOL
				char *var = malloc(assigner * sizeof(char));
				strncpy(var, cmd, assigner);
				var[assigner] = 0;
				assigner++;
				int diff = strlen(cmd) - assigner;
				char *value = malloc((diff + 1) * sizeof(char));
				strncpy(value, cmd + assigner, diff);
				value[diff] = 0;
				FILE *fp = fopen(var, "w+");
				fprintf(fp, value);
				fclose(fp);
				free(var);
				free(value);
				return 0;
			}
			for (int ii = 0; ii < strlen(cmd); ++ii) {
				if (cmd[ii] == ' ' || cmd[ii] == '=') {
					com_cmd[ii] = 0;
					break;
				} else {
					com_cmd[ii] = cmd[ii];
				}
			}

			if (strcmp(com_cmd, "cd") == 0) {
				int cmd_len = strlen(com_cmd) + 1;
				for (int ii = cmd_len; ii < strlen(cmd); ++ii) {
					com_args[ii - cmd_len] = cmd[ii];
				}
				com_args[strlen(cmd) - cmd_len] = '\0';
				code = chdir(com_args);
				return code;
			}
		}
	} else if (strcmp(nodes->op, ";") == 0) {
		if (nodes->count == 1) {
			code = execute(nodes->tok_one);
			return code;
		} else {
			execute(nodes->tok_one);
			code = execute(nodes->tok_two);
			return code;
		}
	} else if (strcmp(nodes->op, "||") == 0) {
		code = execute(nodes->tok_one);
		if (code != 0) {
			code = execute(nodes->tok_two);
		}
		return code;
	} else if (strcmp(nodes->op, "&&") == 0) {
		code = execute(nodes->tok_one);
		if (code == 0) {
			code = execute(nodes->tok_two);
		}
		return code;
	}

	if ((cpid = fork())) {
		int status;
		code = 0;
		if (strcmp(nodes->op, "=") == 0) {
			waitpid(cpid, &status, 0);
			code = status;
		} else if (strcmp(nodes->op, "&") == 0) {
			if (nodes->count == 1) {
				return 0;
			} else {
				execute(nodes->tok_two);
				return code;
			}
		} else if (strcmp(nodes->op, ">") == 0) {
			waitpid(cpid, &status, 0);
			code = status;
		} else if (strcmp(nodes->op, "<") == 0) {
			waitpid(cpid, &status, 0);
			code = status;
		} else if (strcmp(nodes->op, "|") == 0) {
			waitpid(cpid, &status, 0);
			code = status;
		}

		return code;
	} else {
		char com_args[256];
		char com_cmd[256];
		if (strcmp(nodes->op, "=") == 0) {
			char *cmd = nodes->command;
			if (strcmp(cmd, "cd") == 0) {
				exit(0);
			}

			if (cmd[strlen(cmd) - 1] == '\n') {
				cmd[strlen(cmd) - 1] = '\0';
			}
			int quoteCounter = 0;
			if (cmd[0] == '(') {
				char *parens = malloc(50);
				int ii = 0;
				int nn = 1;
				int deep = 1;
				while (1) {
					if (cmd[nn + ii] == '"') {
						quoteCounter = 1 - quoteCounter;
					} else if (cmd[nn + ii] == '(') {
						deep = deep + 1;
					} else if ((quoteCounter == 0) && (cmd[nn + ii] == ')')) {
						deep = deep - 1;
					}
					nn = nn + 1;
					if (deep < 1) {
						break;
					}
				}

				memcpy(parens, cmd + 1, nn - 2);
				parens[nn - 2] = '\0';
				liner(parens);
				exit(0);
			}

			int index = 0;
			int quoteSwitch = 0;
			for (int ii = 0; ii <= (strlen(cmd)); ++ii) {
				if (cmd[ii] == '"') {
					if (quoteSwitch == 0) {
						index++;
					}
					quoteSwitch = 1 - quoteSwitch;
				} else if ((quoteSwitch == 0) && (isspace(cmd[ii]) || cmd[ii] == '\0')) {
					index++;
				}
			}
			int size = index + 1;
			assert(index > 0);
			char **comArgs = calloc(index, sizeof(char*));
			char command[32];
			char *argv[size];
			int jj = 0;
			index = -1;
			quoteSwitch = 0;
			int assist = 0;
			for (int ii = 0; ii <= (strlen(cmd)); ++ii) {
				if (assist != 0 && ii == strlen(cmd)) {
					if (jj < 16) {
						comArgs[index][jj] = '\0';
					}
				} else if (cmd[ii] == '"') {
					quoteSwitch = 1 - quoteSwitch;
					if (quoteSwitch == 0) {
						comArgs[index][jj] = '\0';
						assist = 1;
					} else {
						comArgs[index] = malloc(16 * sizeof(char));
						jj = 0;
					}
				} else if ((quoteSwitch == 0) && (isspace(cmd[ii]) || cmd[ii] == '\0')) {
					if (index != -1) {
						comArgs[index][jj] = '\0';
					} else {
						command[jj] = '\0';
					}
					index++;
					jj = 0;
					assist = 0;
					comArgs[index] = malloc(16 * sizeof(char));
				} else {
					if (assist == 1) {
						comArgs[index][jj] = '\0';
					} else if (index != -1) {
						if (isspace(cmd[ii])) {
							comArgs[index][jj] = ' ';
						} else {
							comArgs[index][jj] = cmd[ii];
						}
					} else {
						command[jj] = cmd[ii];
					}
					jj++;
				}
			}

			argv[0] = command;
			for (int ii = 1; ii < size - 1; ++ii) {
				argv[ii] = comArgs[ii - 1];
			}
			argv[size - 1] = '\0';

			if (strcmp(command, "echo") == 0) {
				for (int ii = 1; ii < size - 1; ++ii) { // makes sure none of the commands are vars, if are swap
					char *argument = malloc(50);
					if (argv[ii] != 0) {
						strncpy(argument, argv[ii], strlen(argv[ii]));
						argument[strlen(argv[ii])] = 0;
						if (argument[0] == '$') {
							int epicSize = sizeof(argv[ii]) - sizeof(char);
							char *var = malloc(epicSize + 1);
							strncpy(var, argv[ii] + 1, epicSize);
							var[epicSize] = 0;
							FILE *fp = fopen(var, "r");
							char buff[255];
							fgets(buff, 10, fp);
							free(argv[ii]);
							int lengthEpic = strlen(buff);
							argv[ii] = calloc(lengthEpic + 1, sizeof(char));
							strncpy(argv[ii], buff, lengthEpic);
							argv[ii][lengthEpic] = 0;
							fclose(fp);
						}
					}
					free(argument);
				}
				execvp(command, argv);
			}

			if (strcmp(command, "pwd") == 0) {
				char *argvs[] = { command, NULL };
				int code = execvp(command, argvs);
				exit(code);
			}

			int code = execvp(command, argv);
			exit(code);
		} else if (strcmp(nodes->op, "&") == 0) {
			char *cmd = nodes->tok_one->command;

			if (cmd[strlen(cmd) - 1] == '\n') {
				cmd[strlen(cmd) - 1] = '\0';
			}

			if (cmd[0] == '(') {
				char *parens = malloc(50);
				int ii = 0;
				int nn = 1;
				int deep = 1;
				while (1) {
					if (cmd[nn + ii] == '(') {
						deep = deep + 1;
					} else if (cmd[nn + ii] == ')') {
						deep = deep - 1;
					}
					nn = nn + 1;
					if (deep < 1) {
						break;
					}
				}

				memcpy(parens, cmd + 1, nn - 2);
				parens[nn - 2] = '\0';
				liner(parens);
				exit(0);
			}

			int index = 0;
			for (int ii = 0; ii <= (strlen(cmd)); ++ii) {
				if (isspace(cmd[ii]) || cmd[ii] == '\0') {
					index++;
				}
			}
			int size = index + 1;
			assert(index > 0);
			char **comArgs = calloc(index, sizeof(char*));
			char command[32];
			char *argv[size];
			int jj = 0;
			index = -1;
			comArgs[index] = malloc(16 * sizeof(char));
			for (int ii = 0; ii <= (strlen(cmd)); ++ii) {
				if (isspace(cmd[ii]) || cmd[ii] == '\0') {
					if (index != -1) {
						comArgs[index][jj] = '\0';
					} else {
						command[jj] = '\0';
					}
					index++;
					jj = 0;
					comArgs[index] = malloc(16 * sizeof(char));
				} else {
					if (index != -1) {
						comArgs[index][jj] = cmd[ii];
					} else {
						command[jj] = cmd[ii];
					}
					jj++;
				}
			}
			argv[0] = command;
			for (int ii = 1; ii < size - 1; ++ii) {
				argv[ii] = comArgs[ii - 1];
			}
			argv[size - 1] = '\0';
			int code = execvp(command, argv);
			exit(code);
		} else if (strcmp(nodes->op, ">") == 0) {
			close(1);
			char *file_name = nodes->tok_two->command;
			int file = open(file_name, O_WRONLY | O_CREAT | O_TRUNC, 0644);
			dup(file);
			char *cmd = nodes->tok_one->command;
			if (cmd[strlen(cmd) - 1] == '\n') {
				cmd[strlen(cmd) - 1] = '\0';
			}

			int index = 0;
			for (int ii = 0; ii <= (strlen(cmd)); ++ii) {
				if (isspace(cmd[ii]) || cmd[ii] == '\0') {
					index++;
				}
			}
			int size = index + 1;
			assert(index > 0);
			char **comArgs = calloc(index, sizeof(char*));
			char command[32];
			char *argv[size];
			int jj = 0;
			index = -1;
			comArgs[index] = malloc(16 * sizeof(char));
			for (int ii = 0; ii <= (strlen(cmd)); ++ii) {
				if (isspace(cmd[ii]) || cmd[ii] == '\0') {
					if (index != -1) {
						comArgs[index][jj] = '\0';
					} else {
						command[jj] = '\0';
					}
					index++;
					jj = 0;
					comArgs[index] = malloc(16 * sizeof(char));
				} else {
					if (index != -1) {
						comArgs[index][jj] = cmd[ii];
					} else {
						command[jj] = cmd[ii];
					}
					jj++;
				}
			}
			argv[0] = command;
			for (int ii = 1; ii < size - 1; ++ii) {
				argv[ii] = comArgs[ii - 1];
			}
			argv[size - 1] = '\0';

			int code = execvp(command, argv);
			exit(code);
		} else if (strcmp(nodes->op, "<") == 0) {
			close(0);
			char *file_name = nodes->tok_two->command;
			int file = open(file_name, O_RDONLY);
			dup(file);
			char *cmd = nodes->tok_one->command;
			if (cmd[strlen(cmd) - 1] == '\n') {
				cmd[strlen(cmd) - 1] = '\0';
			}

			int index = 0;
			for (int ii = 0; ii <= (strlen(cmd)); ++ii) {
				if (isspace(cmd[ii]) || cmd[ii] == '\0') {
					index++;
				}
			}
			int size = index + 1;
			assert(index > 0);
			char **comArgs = calloc(index, sizeof(char*));
			char command[32];
			char *argv[size];
			int jj = 0;
			index = -1;
			comArgs[index] = malloc(16 * sizeof(char));
			for (int ii = 0; ii <= (strlen(cmd)); ++ii) {
				if (isspace(cmd[ii]) || cmd[ii] == '\0') {
					if (index != -1) {
						comArgs[index][jj] = '\0';
					} else {
						command[jj] = '\0';
					}
					index++;
					jj = 0;
					comArgs[index] = malloc(16 * sizeof(char));
				} else {
					if (index != -1) {
						comArgs[index][jj] = cmd[ii];
					} else {
						command[jj] = cmd[ii];
					}
					jj++;
				}
			}
			argv[0] = command;
			for (int ii = 1; ii < size - 1; ++ii) {
				argv[ii] = comArgs[ii - 1];
			}
			argv[size - 1] = '\0';

			int code = execvp(command, argv);
			exit(code);
		} else if (strcmp(nodes->op, "|") == 0) {
			int pipes[2];
			pipe(pipes);
			int cpid1;
			if ((cpid1 = fork())) {
				close(pipes[1]); // close the writing end, stout
				close(0); // closes standard in
				dup(pipes[0]);
				close(pipes[0]);
				execute(nodes->tok_two);
				exit(0);
			} else {
				close(pipes[0]); // read end, standard in takes
				close(1); // closes stout
				dup(pipes[1]);
				close(pipes[1]);
				execute(nodes->tok_one);
				exit(0);
			}

		}
		exit(0);
	}
}

void chomp(char *text) {
	if (text[strlen(text) - 1] == '\n') {
		text[strlen(text) - 1] = '\0';
	}
}

void liner(char *cmd) {
	linked_list *tokens_rev = tokenize(cmd);
	linked_list *tokens = rev_free(tokens_rev);
	int len = length(tokens);
	node **nodes = calloc(len, sizeof(node*));
	int switcher = 0;
	int counter = 0;
	int oldCounter = -1;
	linked_list *tokens_copy = tokens;
	char *temp = malloc(256 * sizeof(char));
	int charCounter = 0;
	strcpy(temp, "");
	for (; tokens; tokens = tokens->tail) {
		if (!is_operator(tokens->head[0])) {
			if (oldCounter != counter) {
				oldCounter = counter;
				nodes[counter] = malloc(sizeof(node));
				nodes[counter]->op = (char*) malloc(4);
				strcpy(nodes[counter]->op, "=");
				nodes[counter]->count = 0;
				nodes[counter]->used = 1;
			}
			strcat(temp, tokens->head);
			charCounter += strlen(tokens->head);
			if (tokens->tail != 0) {
				if (is_operator(tokens->tail->head[0]) != 1) {
					strcat(temp, " ");
					charCounter++;
				} else {
					temp[charCounter] = '\0';
					nodes[counter]->command = malloc(
							(charCounter + 1) * sizeof(char));
					strcpy(nodes[counter]->command, temp);
					nodes[counter]->command[charCounter] = '\0';
					charCounter = 0;
					free(temp);
					temp = malloc(256 * sizeof(char));
				}
			} else {
				nodes[counter]->command = malloc(
						(charCounter + 1) * sizeof(char));
				strcpy(nodes[counter]->command, temp);
				nodes[counter]->command[charCounter] = '\0';

				charCounter = 0;
				free(temp);
				temp = malloc(256 * sizeof(char));
			}
		} else {
			counter++;
			nodes[counter] = malloc(sizeof(node));
			nodes[counter]->op = (char*) malloc(8);
			strcpy(nodes[counter]->op, tokens->head);
			if (tokens->tail != 0) {
				nodes[counter]->count = 2;
				nodes[counter]->used = 0;
				counter = counter + 1;
			} else {
				nodes[counter]->count = 1;
				nodes[counter]->used = 0;
			}
		}
	}
	free(temp);
	int count = counter + 1;
	free_linked_list(tokens_copy);
	node **nodesTemp = calloc(count, sizeof(node*));
	for (int ii = 0; ii < count; ++ii) {
		nodesTemp[ii] = nodes[ii];
	}
	free(nodes);
	nodes = calloc(count, sizeof(node*));
	for (int ii = 0; ii < count; ++ii) {
		nodes[ii] = nodesTemp[ii];
	}

	int nodeCounter = -1;
	int nodeCounts = 0;
	while (count > 1) {
		nodeCounter = -1;
		for (int ii = 0; ii < count; ++ii) {
			assert(nodes[ii]);
			if (strcmp(nodes[ii]->op, "<") == 0 && nodes[ii]->used == 0) {
				nodeCounter = ii;
				nodeCounts = 2;
				nodes[ii]->tok_one = nodes[ii - 1];
				nodes[ii]->tok_two = nodes[ii + 1];
				nodes[ii]->used = 1;
				break;
			} else if (strcmp(nodes[ii]->op, ">") == 0
					&& nodes[ii]->used == 0) {
				nodeCounter = ii;
				nodeCounts = 2;
				nodes[ii]->tok_one = nodes[ii - 1];
				nodes[ii]->tok_two = nodes[ii + 1];
				nodes[ii]->used = 1;
				break;
			}
		}
		if (nodeCounter == -1) {
			for (int ii = 0; ii < count; ++ii) {
				if (strcmp(nodes[ii]->op, "|") == 0 && nodes[ii]->used == 0) {
					nodeCounter = ii;
					nodeCounts = 2;
					nodes[ii]->tok_one = nodes[ii - 1];
					nodes[ii]->tok_two = nodes[ii + 1];
					nodes[ii]->used = 1;
					break;
				}
			}
		}
		if (nodeCounter == -1) {
			for (int ii = 0; ii < count; ++ii) {
				if (strcmp(nodes[ii]->op, "&&") == 0 && nodes[ii]->used == 0) {
					nodeCounter = ii;
					nodeCounts = 2;
					nodes[ii]->tok_one = nodes[ii - 1];
					nodes[ii]->tok_two = nodes[ii + 1];
					nodes[ii]->used = 1;
					break;
				} else if (strcmp(nodes[ii]->op, "||") == 0
						&& nodes[ii]->used == 0) {
					nodeCounter = ii;
					nodeCounts = 2;
					nodes[ii]->tok_one = nodes[ii - 1];
					nodes[ii]->tok_two = nodes[ii + 1];
					nodes[ii]->used = 1;
					break;
				}
			}
		}
		if (nodeCounter == -1) {
			for (int ii = 0; ii < count; ++ii) {
				if (strcmp(nodes[ii]->op, "&") == 0 && nodes[ii]->used == 0) {
					nodeCounter = ii;
					nodeCounts = 1;
					if (nodes[ii]->count == 2) {
						nodeCounts++;
						nodes[ii]->tok_one = nodes[ii - 1];
						nodes[ii]->tok_two = nodes[ii + 1];
					} else {
						nodes[ii]->tok_one = nodes[ii - 1];
					}
					nodes[ii]->used = 1;
					break;
				}
			}
		}

		if (nodeCounter == -1) {
			for (int ii = 0; ii < count; ++ii) {
				if (strcmp(nodes[ii]->op, ";") == 0 && nodes[ii]->used == 0) {
					nodeCounter = ii;
					if (nodes[ii]->count == 2) {
						nodeCounts = 2;
						nodes[ii]->tok_one = nodes[ii - 1];
						nodes[ii]->tok_two = nodes[ii + 1];
					} else {
						nodeCounts = 1;
						nodes[ii]->tok_one = nodes[ii - 1];
					}
					nodes[ii]->used = 1;
					break;
				}
			}
		}
		free(nodesTemp);
		assert(count > nodeCounts);
		nodesTemp = calloc((count - nodeCounts), sizeof(node*));
		int index = 0;
		for (int ii = 0; ii < count; ++ii) {
			if (nodeCounts == 2) {
				if (ii + 1 == nodeCounter || ii - 1 == nodeCounter) {
					continue;
				}
			} else if (nodeCounts == 1) {
				if (ii + 1 == nodeCounter) {
					continue;
				}
			}
			nodesTemp[index] = nodes[ii];
			index++;
		}
		count = count - nodeCounts;
		free(nodes);
		nodes = calloc(count, sizeof(node*));
		for (int ii = 0; ii < count; ++ii) {
			nodes[ii] = nodesTemp[ii];
		}
	}

	free(nodesTemp);
	execute(nodes[0]);
	free_nodes(nodes[0]);
	free(nodes);
}

int is_empty(char *s) {
	int ii = 0;
	while (s[ii] != '\0') {
		if (!isspace(s[ii])) {
			return 0;
		}
		ii++;
	}
	return 1;
}

int main(int argc, char *argv[]) {
	if (argc != 1) {
		FILE *fh = fopen(argv[1], "r");
		if (!fh) {
			puts("error");
		} else {
			char temp[128];
			while (1) {
				char *line = fgets(temp, 128, fh);
				if (!line) {
					break;
				}

				if (line[strlen(line) - 2] == '\\') {
					char *line2 = malloc(strlen(line) * sizeof(char));
					strcpy(line2, line);
					line = fgets(temp, 128, fh);
					char *conc = calloc((strlen(line2)+ strlen(line)) - 1, sizeof(char));
					strncat(conc, line2, strlen(line2) * sizeof(char) - 2);
					strncat(conc, line, strlen(line) * sizeof(char));
					conc[strlen(line) + strlen(line2) - 2] = 0;
					chomp(conc);
					liner(conc);
					free(conc);
					free(line2);
					return 0;
				}
				chomp(line);
				liner(line);
			}
			fclose(fh);
		}
	} else if (argc == 1) {

		char cmd[256];
		while (1) {
			printf("nush$ ");
			fflush(stdout);
			fgets(cmd, 256, stdin);
			if (feof(stdin)) {
				puts("\n");
				exit(0);
			}

			if (is_empty(cmd) == 1) {
				continue;
			}
			cmd[strcspn(cmd, "\n")] = 0;

			liner(cmd);
		}
	}

	return 0;
}
