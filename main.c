#include <stdio.h>
#include <signal.h> // handles `ctrl-c` exit
// #include <limits.h> // useful for getting length of path
#include <string.h>
#include <curl/curl.h>

// for creating directory cross platform
#ifdef _WIN32
	#include <direct.h>
	#define CREATE_DIR(path) _mkdir(path)
	#define DIR_EXISTS(path) (GetFileAttributes(path) != INVALID_FILE_ATTRIBUTES)
#elif __linux__ || __APPLE__
	#include <sys/stat.h>
	#include <dirent.h>
	#define DIR_EXISTS(path) (opendir(path) != NULL)
	#define CREATE_DIR(path) mkdir(path, 0777)
#else
	#error "Unsupported platform"
#endif

// for checking existance of file
#include <sys/stat.h>

#define PROMPT_MAX_LENGTH 256
#define PROMPT_SCALING 4

static char tmp_prompt[PROMPT_MAX_LENGTH],
						path_to_save[PROMPT_MAX_LENGTH * 2],
						prompt[PROMPT_MAX_LENGTH * (PROMPT_SCALING - 1)],
						url[PROMPT_MAX_LENGTH * PROMPT_SCALING],
						*tmp_prompt_cpy = NULL;

static int mode, tmp_prompt_length, close_loop = 0;

// Signal handler function
void handle_signal(int signal)
{
	if (signal == SIGINT) {
		printf("Ctrl-C pressed. Terminating the program...\n");
		// Add any cleanup code here if needed
		close_loop = 1;
	}
}

void input_mode()
{
	// ask for mode
	do {
		printf("Enter mode\n\t1 -> Different prompts\n\t2 -> Same prompt variations\n\t(`Ctrl-C` to quit): ");
	} while (
		(scanf("%d", &mode) != 1)
		|| mode < 1 || mode > 2);
	getchar();
}

// Format prompt, this at the moment clears the spaces in the prompt with
// %20 which allows us to avoid the invalid URL error when given in it
// ALSO OUTPUTS NUMBER OF CHARACTERS IN tmp_prompt
int remove_spaces()
{
	// Remove newline character from prompt
	tmp_prompt[strcspn(tmp_prompt, "\n")] = '\0';
	int promptIndex = 0;
	int tmp_promptLength = strlen(tmp_prompt);
/* Required if not properly assigned length to prompt string *
 *
 *	int maxPromptCopyLength = PROMPT_MAX_LENGTH - 1;

 *	for (int i = 0; i < tmp_promptLength; i++) {
 *		if(promptIndex < maxPromptCopyLength) {
 *			if (tmp_prompt[i] == ' ') {
 *				if (promptIndex + 3 > maxPromptCopyLength) goto end;
 *				prompt[promptIndex++] = '%';
 *				prompt[promptIndex++] = '2';
 *				prompt[promptIndex++] = '0';
 *			} else prompt[promptIndex++] = tmp_prompt[i];
 *		} else goto end;
 *	}
 *	prompt[promptIndex] = '\0';
 *	return 0;
 *
 *end:
 *	prompt[promptIndex] = '\0';
 *	printf("WARNING: Prompt length overflow!\n");
 *	return 1;
 */
	// If proper length is assigned to prompt with respect to tmp_prompt
	// including null character
	for (int i = 0; i < tmp_promptLength; ++i) {
		if (tmp_prompt[i] == ' ') {
			prompt[promptIndex++] = '%';
			prompt[promptIndex++] = '2';
			prompt[promptIndex++] = '0';
		} else prompt[promptIndex++] = tmp_prompt[i];
	}
	return tmp_promptLength + 1;
}

void handle_prompt_and_url(CURL *curl)
{
	printf("tmp_prompt: %s\n", tmp_prompt);

	// API GET request URL
	snprintf(url, sizeof url, "https://image.pollinations.ai/prompt/%s", prompt);
	printf("Get request to url: %s\n", url);

	// Set the URL with curl
	curl_easy_setopt(curl, CURLOPT_URL, url);
	curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L); // Disable certificate verification
}

int handle_directory()
{
	if (DIR_EXISTS(tmp_prompt_cpy)) {
			printf("Directory already exists.\n");
			return 0;
	} else {
			// Create the directory
			int result = CREATE_DIR(tmp_prompt_cpy);

			if (result == 0) {
					printf("Directory created successfully.\n");
					return 0;
			} else {
					printf("Failed to create the directory.\n");
					return 1;
			}
	}
}

int handle_file_and_process(CURL *curl, CURLcode res)
{
	struct stat buffer;
	if (stat(path_to_save, &buffer) == 0) {
		printf("File \"%s\" already exists!\n", path_to_save);
		return 0;
	}
	// printf("path_to_save: %s\n", path_to_save);
	FILE *fp = fopen(path_to_save, "wb");
	if (fp == NULL) {
		printf("Error opening file.\n");
		return 1;
	}

	// Set file as the output
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, fp);

	// Perform the request
	printf("Processing...\n");
	res = curl_easy_perform(curl);

	if (res != CURLE_OK) {
		printf("Request failed: %s\n", curl_easy_strerror(res));
		return 1;

	}
	printf("Image saved for prompt: %s\n", tmp_prompt);
	// Clean up opened file
	fclose(fp);

	return 0;
}

int handle_mode(CURL *curl,	CURLcode res)
{
	switch (mode) {
		case 1:	{
			while(!close_loop) {
				remove_spaces();
				handle_prompt_and_url(curl);

				snprintf(path_to_save, sizeof path_to_save, "./images/%s.jpg", tmp_prompt);

				if(handle_file_and_process(curl, res)) return 0;

				// ask for prompt
				printf("Enter a prompt (`Ctrl-C` to quit): ");
				fgets(tmp_prompt, PROMPT_MAX_LENGTH, stdin);

				// resetting the curl instance for reusing it
				curl_easy_reset(curl);
			}
			break;
		}

		case 2: {
			int tmp_prompt_cpy_length = PROMPT_MAX_LENGTH + 64;
			tmp_prompt_cpy = malloc(tmp_prompt_cpy_length);
			strncpy(tmp_prompt_cpy, tmp_prompt, PROMPT_MAX_LENGTH);
			snprintf(tmp_prompt_cpy, tmp_prompt_cpy_length, "./images/%s", tmp_prompt);
			// Remove newline character from tmp_prompt_cpy
			tmp_prompt_cpy[strcspn(tmp_prompt_cpy, "\n")] = '\0';
			if(handle_directory()) return 1;

			while(!close_loop) {
				tmp_prompt_length = remove_spaces();
				handle_prompt_and_url(curl);

				snprintf(path_to_save, sizeof path_to_save, "%s/%s.jpg", tmp_prompt_cpy, tmp_prompt);
				printf("path_to_save: %s\n", path_to_save);

				if(handle_file_and_process(curl, res)) return 1;

				if(tmp_prompt_length + 1 > PROMPT_MAX_LENGTH) close_loop = 1;
				else {
					tmp_prompt[tmp_prompt_length - 1] = ' ';
					tmp_prompt[tmp_prompt_length] = '\n';
				}

				// resetting the curl instance for reusing it
				curl_easy_reset(curl);
			}
			free(tmp_prompt_cpy);
			break;
		}
	}
	return 0;
}

int main()
{
	// Register the signal handler
  signal(SIGINT, handle_signal);

	CURL *curl;
	CURLcode res = 0;

	// Initialize libcurl
	curl_global_init(CURL_GLOBAL_DEFAULT);
	curl = curl_easy_init();

	if (!curl) {
		printf("ERROR: Curl initialization failed!\n");
		exit(0);
	}

	input_mode();

	printf("Enter a prompt (`Ctrl-C` to quit): ");
	fgets(tmp_prompt, PROMPT_MAX_LENGTH, stdin);

	// loop function
	handle_mode(curl, res);

	// Cleanup libcurl
	curl_easy_cleanup(curl);

	// Cleanup libcurl global state
	curl_global_cleanup();

	return 0;
}
