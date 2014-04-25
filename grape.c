#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define INIT_BUF_SIZE 128  //the initial buffer size when reading a line.

char *errorString = "";

/*
	The length of the first component of regex.  returns -1 if the string ends prematurely.
*/
inline int headLen(char *regex) {
	switch (*regex) {
		char *r;
		case '\\' : return 2;
		case '[' :
			r = regex+1;
			if (*r == '^') {
				r++;
			}
			while (*r && *++r != ']');
			
			if (*r == 0) {
				errorString = "Unexpected token: [";
				exit(1);
			}
			return r+1-regex;
		case '(' :
			r = regex + 1;
			for (r = regex + 1; *r && *r != ')'; r++) {
				if (*r == '(') {
					r += headLen(r) - 1; //- 1 because we increment after the loop
				}
			}
			if (*r == 0) {
				errorString = "Unexpected token: (";
				exit(1);
			}
			return *r ? r+1-regex : -1;
		default: return 1;
	}
}


int matchFront(char *start, char *str, char *regex) {
	if (*regex == 0 || *regex == '|') {
		return 0;
	}
	if (*str == 0) {
		return -1;
	}
	char *end = str + strlen(str) - 1;
	int hl = headLen(regex);
	int altHl = hl+1;	//head length including suffix
	int ml = 1;	//match length
	char suffix = regex[hl];
	int rest;
	int replacedPlus = 0;
	int sub = 0;
	int i;
	switch (*regex) {
		case '(':
			regex[hl-1] = 0;
			while (str[sub] || str[sub-1]) {
				char c = str[sub];
				str[sub] = 0;
				ml = matchFront(str, str, regex+1);
				str[sub] = c;
				if (ml >= 0) {
					rest = matchFront(str, str + ml, regex + hl);
					if (rest >= 0) {
						regex[hl-1] = ')';
						return ml + rest;
					}
				}
				sub++;
			}
			regex[hl-1] = ')';
			return -1;
		case '[':
			i = 1;
			if (regex[i] == '^') {
				i++;
				do {
					if (regex[i] == *str) {
						goto noMatch;
					}
					i++;
				}	while (regex[i] != ']');
				goto match;
			} else {
				do {
					if (regex[i] == *str) {
						goto match;
					}
					i++;
				}	while (regex[i] != ']');
				goto noMatch;
			}
			
			
		case '.':	//always match, except on end of string
			if (*str == 0) {
				goto noMatch;
			}
			break;
		
		case '\\':
			regex++;	//fall through here.
			hl--;
			
		default:
			if (*str != *regex) {
				goto noMatch;
			}
	}
	match:
	
	switch (suffix) {
		case '?':
			hl++;
			break;
			
		case '+':
			replacedPlus = hl;
			regex[replacedPlus] = '*';
		case '*':
			hl = 0;
	}

	rest = matchFront(start, str+ml, regex+hl);
	if (replacedPlus) {
		regex[replacedPlus] = '+';
	}
	if (rest < 0) {
		goto noMatch;
	}
	return rest + ml;
	
	restFail:
	if (*regex == '(') {
		
	}
	
	noMatch:
	if (suffix == '*' || suffix == '?') {
		return matchFront(start, str, regex + altHl);
	}
	while (*regex) {
		regex += headLen(regex);
		if (*regex == '|') {
			return matchFront(start, start, regex + 1);
		}
	}
	return -1;
}

int match(char *str, char *regex) {
	if (matchFront(str, str, regex) >= 0) return 1;
	if (*str == 0) return 0;
	return match(str + 1, regex);
}

void printError(void) {
	printf("%s\n", errorString);
}


int main(int argc, char** argv) {
	atexit(printError);
	char *regex;
	FILE* fp = stdin;
	if (argc < 2) {
		return 0;
	}
	
	regex = argv[1];
	int lineSize = INIT_BUF_SIZE;
	char *line = (char *) malloc(lineSize);
	while(1) {
		int n = 0;
		char c;
		while (!feof(fp) && (c = (char) fgetc(fp)) != '\n') {
			if (n == lineSize) {
				lineSize *= 2;
				line = realloc(line, lineSize);
			}
			line[n] = c;
			n++;
		}
		line[n] = 0;
		if (match(line, regex)) {
			printf("%s\n", line);
		}
	}
}

