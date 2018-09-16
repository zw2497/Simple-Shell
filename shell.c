/*
 * SPDX-License-Identifier: 0BSD
 * Shell.c
 * Shell
 *
 * Created by Zhicheng WU on 9/10/2018
 * Copyright 2018 All rights reserved
 */

#include <stdio.h>
#include "string.h"
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <wait.h>
#include <inttypes.h>
#include <stdbool.h>
#include <ctype.h>
#include <shell.h>

struct memo {
int offset;
char *args;
};

struct list {
int first;
int end;
struct memo **m;
int size;
};



struct list *list1;
int i;

int listnum(int n)
{
	if (n >= 0)
		n = n % HISTORY_SIZE;
	else
		n = n + HISTORY_SIZE;
	return n;
}

bool isInteger(char *test)
{
	int result;
	int p = 0;

	while (test[p] != '\0') {
		result = isdigit(test[p]);
		if (result == 0)
			break;
		p++;
	}
	return result;
}

bool startsWith(const char *pre, const char *str)
{
	size_t lenpre = strlen(pre);
	size_t lenstr = strlen(str);

	return lenstr < lenpre ? false : strncmp(pre, str, lenpre) == 0;
}

void deloneElement(void)
{
	if (list1->size == 0)
		return;
	list1->size = list1->size - 1;
	list1->end = list1->end - 1;
	if (list1->end < 0)
		list1->end = list1->end + HISTORY_SIZE;
	i--;


}

void printHistory(int n)
{
	int p;

	if (n > list1->size)
		n = list1->size;
	else if (n < 0)
		n = 0;
	p = (list1->first + (list1->size - n)) % HISTORY_SIZE;
	while (n > 0) {
		printf("%d %s", list1->m[p]->offset, list1->m[p]->args);
		p = (p + 1) % HISTORY_SIZE;
		n--;
	}
}

void deleteHistory(void)
{
	list1->first = list1->end;
	list1->size = 0;
}


void addToList(int i, struct list *l, char **argument)
{

	struct memo *m = malloc(sizeof(struct memo));

	m->args = malloc(100 * sizeof(char));
	strcpy(m->args, *argument);
	m->offset = i;


	i = i % HISTORY_SIZE;
	l->m[i] = m;
	l->end = (l->end + 1) % HISTORY_SIZE;
	if (l->first == l->end)
		l->first = (l->first + 1) % HISTORY_SIZE;
	else
		list1->size++;
}



char *concat(const char *s1, const char *s2)
{
	char *result = malloc(strlen(s1) + strlen(s2) + 1);

	strcpy(result, s1);
	strcat(result, s2);
	return result;
}

void inputStdin(char **input, size_t *n)
{
	if (getline(input, n, stdin) == -1) {
		fprintf(stderr, "error: %s\n", strerror(errno));
		exit(EXIT_FAILURE);
	}
}

char **tokenize(char *input, char **argument, int size)
{
	const char *delimiter = "\n ";
	int i = 0;
	char *temp = malloc(strlen(input) + 1);

	strcpy(temp, input);


	argument[0] = strtok(temp, delimiter);
	while (argument[i] != NULL) {
		i++;
		argument[i] = strtok(NULL, delimiter);
		if (i > size - 2) {
			size = size * 2;
			argument = realloc(argument, size * sizeof(char *));
		}
	}
	return argument;


}

void exe(char **args)
{
	int pid;
	int status;


	pid = fork();
	switch (pid) {
	case 0:
		if (execv(args[0], args) == -1) {
			fprintf(stderr, "error: %s\n", strerror(errno));
			exit(EXIT_FAILURE);
		}
		exit(EXIT_SUCCESS);
		break;
	case -1:
		fprintf(stderr, "error: %s\n", strerror(errno));
		exit(EXIT_FAILURE);
		break;
	default:
		if (waitpid(pid, &status, 0) != pid)
			status = -1;
	}


}

void exenofork(char **args)
{
	if (execv(args[0], args) == -1) {
		fprintf(stderr, "error: %s\n", strerror(errno));
		exit(EXIT_FAILURE);
	}
	exit(EXIT_SUCCESS);

}

void exitProcess(char **args)
{
	free(args);
	exit(EXIT_SUCCESS);
}

void cdProcess(char **args)
{
	if (chdir(args[1]) == -1)
		fprintf(stderr, "error: %s\n", strerror(errno));
}

void historyProcess(char **args)
{
	if (args[1] == NULL) {
		printHistory(list1->size);
	} else if (strcmp(args[1], "-c") == 0) {
		deleteHistory();
	} else if (isInteger(args[1])) {
		uintmax_t num = strtoumax(args[1], NULL, 10);

		if (num == UINTMAX_MAX && errno == ERANGE)
			fprintf(stderr, "error: invalid command\n");
		printHistory(num);
	} else {
		fprintf(stderr, "error: invalid command\n");
	}
}

char *bang2Process(char **args)
{
	int num = list1->end - 2;
	int n = list1->size - 1;

	while (n) {
		if (startsWith(&args[0][1], list1->m[num]->args)) {
			deloneElement();
			return list1->m[num]->args;
		}
		num = listnum(num - 1);
		n--;
	}
	fprintf(stderr, "error: No match\n");
	deloneElement();
	return *args;
}

char *bang1Process(char **args)
{
	if (list1->size == 1) {
		fprintf(stderr, "error: No history\n");
		deloneElement();
	} else {
		int p;

		p = list1->end - 2;
		if (p < 0)
			p = HISTORY_SIZE + p;
		deloneElement();
		//addToList(i++, list1, &list1->m[p]->args);
		return list1->m[p]->args;
		//run(list1->m[p]->args, arg_new);
	}
	return *args;
}

void pipeProcess(char **args)
{
	char **former;
	char **latter;
	int p = 0;
	pid_t pid;
	int a[10] = {[0 ... 9] = -1};
	int pipenumber;
	int size = 0;
	int c = 0;
	int number = 0;
	int i1 = 0;
	int i2 = 0;


	char **formerargs;
	formerargs = malloc(ARG_NUMBER * sizeof(char *));

	char **latterargs;
	latterargs = malloc(ARG_NUMBER * sizeof(char *));

	while (args[p] != NULL) {
		if (strcmp(args[p], "|") == 0) {
			a[c++] = number;
			size++;
			args[p] = NULL;
			number = 0;
		}
		number++;
		p++;
	}

	pipenumber = c;
	int mypipe[2 * pipenumber];

	for(i = 0; i < pipenumber; i++){
		if(pipe(mypipe + i*2) < 0) {
			perror("couldn't pipe");
			exit(EXIT_FAILURE);
		}
	}

	former = &args[0];
	latter = &args[a[i1] + 1];


	if(bangdetected(former)) {
		tokenize(bangProcess(former), formerargs, ARG_NUMBER);
		former = formerargs;
	}
	if(bangdetected(latter)) {
		tokenize(bangProcess(latter), latterargs, ARG_NUMBER);
		latter = latterargs;
	}

	if (pipe(mypipe)) {
		fprintf(stderr, "Pipe failed.\n");
		exit(EXIT_FAILURE);
	}
	if (pipe(mypipe + 2)) {
		fprintf(stderr, "Pipe failed.\n");
		exit(EXIT_FAILURE);
	}

	pid = fork();

	if (pid == 0) {
		dup2(mypipe[1], STDOUT_FILENO);
		for(i = 0; i < 2 * pipenumber; i++){
			close(mypipe[i]);
		}
		runnofork(former);
	} else if (pid < (pid_t) 0) {
		fprintf(stderr, "Fork failed.\n");
		exit(EXIT_FAILURE);
	} else {
		exepipe(mypipe, latter, a, i1, size, pipenumber, i2);
	}
}

void exepipe(int *file, char **args, int *a, int i1, int size, int pipenumber, int i2)
{
	int pid = fork();
	int status;

	switch (pid) {
		case 0:
			if (size > 1) {
				dup2(file[i2], STDIN_FILENO);
				dup2(file[i2 + 3], STDOUT_FILENO);
				for(i = 0; i < 2 * pipenumber; i++){
					close(file[i]);
				}
				if (execv(args[0], args) == -1) {
					fprintf(stderr, "error: %s\n", strerror(errno));
					exit(EXIT_FAILURE);
				}

			} else {
				dup2(file[i2], STDIN_FILENO);
				for(i = 0; i < 2 * pipenumber; i++){
					close(file[i]);
				}
				if (execv(args[0], args) == -1) {
					fprintf(stderr, "error: %s\n", strerror(errno));
					exit(EXIT_FAILURE);
				}
				exit(EXIT_SUCCESS);
			}

		case -1:
			fprintf(stderr, "error: %s\n", strerror(errno));
			exit(EXIT_FAILURE);
		default:
			if (size > 1) {
				i1++;
				size--;
				i2 = i2 + 2;
				exepipe(file, &args[a[i1]], a, i1, size, pipenumber, i2);
			}
			for(i = 0; i < 2 * pipenumber; i++){
				close(file[i]);
			}
			for(i = 0; i < pipenumber + 1; i++){
				wait(&status);
			}

	}
}

void runnofork(char **args)
{

	if (strcmp(args[0], "exit") == 0)
		exitProcess(args);
	else if (strcmp(args[0], "cd") == 0)
		cdProcess(args);
	else if (strcmp(args[0], "history") == 0)
		historyProcess(args);
	else
		exenofork(args);

}

void runwithfork(char **args)
{
	char *originNew;
	char **argsNew;

	originNew = malloc(ARG_NUMBER * sizeof(char *));
	argsNew = malloc(ARG_NUMBER * sizeof(char *));

	if (strcmp(args[0], "exit") == 0)
		exitProcess(args);
	else if (strcmp(args[0], "cd") == 0)
		cdProcess(args);
	else if (strcmp(args[0], "history") == 0)
		historyProcess(args);
	else if (strcmp(args[0], "!!") == 0){
		originNew = bang1Process(args);
		tokenize(originNew, argsNew, ARG_NUMBER);
		exe(argsNew);
	} else if (args[0][0] == '!' && args[0][1] != '!') {
		originNew = bang2Process(args);
		tokenize(originNew, argsNew, ARG_NUMBER);
		exe(argsNew);
	} else
		exe(args);
}

bool bangdetected(char **args) {
	if (args[0][0] == '!' && args[0][1] != '!')
		return true;
	if (strcmp(args[0], "!!") == 0)
		return true;
	return false;
}


char *bangProcess(char **args)
{
	if (args[0][0] == '!' && args[0][1] != '!')
		return bang2Process(args);
	if (strcmp(args[0], "!!") == 0)
		return bang1Process(args);
}

bool pipedetect(char *origin)
{
	int p1 = 0;

	while (origin[p1] != '\0') {
		if (origin[p1] == '|')
			return true;
		p1++;
	}
	return false;
}

char *pipeaddblank(char *origin)
{
	int p1 = 0;
	int in1;
	char *split1;
	char *split2;


	while (origin[p1] != '\0') {
		if (origin[p1] == '|') {
			split1 = malloc((p1 + 4) * sizeof(char));
			split2 = malloc(
				(strlen(origin) - p1 + 1) * sizeof(char));
			for (int i1 = 0; i1 < p1; i1++)
				split1[i1] = origin[i1];
				split1[p1] = ' ';
				split1[p1 + 1] = '|';
				split1[p1 + 2] = ' ';
				split1[p1 + 3] = '\0';
				in1 = p1 + 1;
			for (int i2 = 0; in1 < strlen(origin); in1++, i2++)
				split2[i2] = origin[in1];
			origin = concat(split1, split2);
			p1++;
		}
		p1++;
	}
	return origin;
}

void run(char *originStr, char **args) {
	if (pipedetect(originStr)) {
		char *newOrigin = pipeaddblank(originStr);
		char **newargs;
		//newargs = malloc(ARG_NUMBER * sizeof(char *));
		newargs = tokenize(newOrigin, newargs, ARG_NUMBER);
		pipeProcess(newargs);
	} else {
		tokenize(originStr, args, ARG_NUMBER);
		runwithfork(args);
	}
}

int main(void)
{
	char *originStr;
	char **args;

	originStr = malloc(ARG_NUMBER * sizeof(char *));
	args = malloc(ARG_NUMBER * sizeof(char *));
	size_t size = 100;

	list1 = malloc(sizeof(struct list));
	list1->m = malloc(HISTORY_SIZE * sizeof(struct memo));
	list1->first = 0;
	list1->end = 0;

	while (1) {
		printf("$");
		inputStdin(&originStr, &size);
		addToList(i++, list1, &originStr);
		run(originStr, args);
	}
}
