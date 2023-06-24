#include <stdio.h>
#include <signal.h> // handles `ctrl-c` exit
#include <string.h>
#include <curl/curl.h>

#define MAX_PROMPT_LENGTH 512
#define MAX_RELATIVE_PATH_LENGTH 576
#define MAX_URL_LENGTH 576

int close = 0;

// Signal handler function
void handle_signal(int signal)
{
	if (signal == SIGINT) {
		printf("Ctrl-C pressed. Terminating the program...\n");
		// Add any cleanup code here if needed
		close = 1;
	}
}

// Format prompt, this at the moment clears the spaces in the prompt with
// %20 which allows us to avoid the invalid URL error when given in it
int remove_spaces(char tmp_prompt[], char prompt[])
{
	int promptIndex = 0;
	int tmp_promptLength = strlen(tmp_prompt);
	int maxPromptCopyLength = MAX_PROMPT_LENGTH - 1;

	for (int i = 0; i < tmp_promptLength; i++) {
		if(promptIndex < maxPromptCopyLength) {
			if (tmp_prompt[i] == ' ') {
				if (promptIndex + 3 > maxPromptCopyLength) goto end;
				prompt[promptIndex++] = '%';
				prompt[promptIndex++] = '2';
				prompt[promptIndex++] = '0';
			} else prompt[promptIndex++] = tmp_prompt[i];
		} else goto end;
	}
	prompt[promptIndex] = '\0';
	return 0;

end:
	prompt[promptIndex] = '\0';
	printf("WARNING: Prompt length overflow!\n");
	return 1;
}

int main()
{
	CURL *curl;
	CURLcode res;

	// Initialize libcurl
	curl_global_init(CURL_GLOBAL_DEFAULT);
	curl = curl_easy_init();

	char tmp_prompt[MAX_PROMPT_LENGTH];
	char prompt[MAX_PROMPT_LENGTH];
	char path_to_save[MAX_RELATIVE_PATH_LENGTH];
	char url[MAX_URL_LENGTH];


	if (!curl) {
		printf("ERROR: Curl initialization failed!\n");
		exit(0);
	}

	// ask for prompt
	printf("Enter a prompt (`Ctrl-C` to quit): ");
	fgets(tmp_prompt, MAX_PROMPT_LENGTH, stdin);

	// main loop starts
	while (!close) {
		// Remove newline character from prompt
		tmp_prompt[strcspn(tmp_prompt, "\n")] = '\0';

		remove_spaces(tmp_prompt, prompt);

		// API GET request URL
		snprintf(url, sizeof url, "https://image.pollinations.ai/prompt/%s", prompt);
		printf("Get request to url: %s\n", url);

		// Set the URL with curl
		curl_easy_setopt(curl, CURLOPT_URL, url);
		curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L); // Disable certificate verification

		// Open file to save the response(with name of given prompt)
		snprintf(path_to_save, sizeof path_to_save, "%s.jpg", tmp_prompt);
		printf("path_to_save: %s\n", path_to_save);

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

		} else {
			printf("Image saved for prompt: %s\n", tmp_prompt);
			// Clean up opened file
			fclose(fp);

			// ask for prompt
			printf("Enter a prompt (`Ctrl-C` to quit): ");
			fgets(tmp_prompt, MAX_PROMPT_LENGTH, stdin);
		}

		// resetting the curl instance for reusing it
		curl_easy_reset(curl);

	}

	// Cleanup libcurl
	curl_easy_cleanup(curl);

	// Cleanup libcurl global state
	curl_global_cleanup();

	return 0;
}
