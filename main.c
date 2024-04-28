#define _GNU_SOURCE
#include <curl/curl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CURL_INIT_FAILED 1
#define ASPRINTF_FAILED 2
#define CURL_EASY_ESCAPE_FAILED 3
#define CURL_FAILED_TO_FETCH_IMAGE 4

CURL *curl;
CURLcode res;

char *prompt, *encoded_prompt, *image, *url;
size_t prompt_length;
const char *url_format = "https://image.pollinations.ai/prompt/%s?width=1920&height=1080&nologo=true";

FILE *fp;

// Signal handler function
void handle_signal(int signal)
{
  if (signal == SIGINT) {
    fprintf(stderr, "Ctrl-C pressed. Terminating the program...\n");
    exit(EXIT_SUCCESS);
  }
}


int main(int argc, char ** argv)
{
  image = url = encoded_prompt = prompt = NULL;

  // register the signal handler
  signal(SIGINT, handle_signal);

  // initialize libcurl
  curl_global_init(CURL_GLOBAL_DEFAULT);
  curl = curl_easy_init();

  if (!curl) {
    fprintf(stderr, "ERROR: Curl initialization failed!\n");
    exit(CURL_INIT_FAILED);
  }

  // taking user's image prompt
  printf("Enter a prompt (`Ctrl-C` to quit): ");
  getline(&prompt, &prompt_length, stdin);

  // remove the newline character from the input's end
  prompt[strcspn(prompt, "\n")] = '\0';
  printf("Entered prompt: %s\n", prompt);

  // encode prompt for url request
  if (!(encoded_prompt = curl_easy_escape(curl, prompt, 0))) {
    fprintf(stderr, "ERROR: curl_easy_escape failed!\n");
    curl_easy_cleanup(curl);
    exit(CURL_EASY_ESCAPE_FAILED);
  }
  // create url request
  if (asprintf(&url, url_format, encoded_prompt) == -1) {
    fprintf(stderr, "ERROR: asprintf failed!\n");
    curl_easy_cleanup(curl);
    exit(ASPRINTF_FAILED);
  }
  // free dynamically allocated memory
  curl_free(encoded_prompt);
  curl_easy_setopt(curl, CURLOPT_URL, url);
  free(url);

  // creating image file for output
  if (asprintf(&image, "%s.jpg", prompt) == -1) {
    fprintf(stderr, "ERROR: asprintf failed!\n");
    curl_easy_cleanup(curl);
    exit(ASPRINTF_FAILED);
  }
  // set up file to save the fetched image
  fp = fopen(image, "wb");

  if (!fp) {
    fprintf(stderr, "Error opening file for writing.\n");
    curl_easy_cleanup(curl);
    return 1;
  }

  // Set the file as the output for libcurl
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, fp);

  // Perform the request
  printf("Requesting Pollinations.ai for \"%s\"\n", prompt);
  res = curl_easy_perform(curl);
  if (res != CURLE_OK) {
    fprintf(stderr, "Failed to fetch image: %s\n", curl_easy_strerror(res));
    fclose(fp);
    curl_easy_cleanup(curl);
    exit(CURL_FAILED_TO_FETCH_IMAGE);
  }
  printf("File saved as \"%s\"\n", image);
  // free dynamically allocated memory
  free(prompt);
  free(image);

  // Clean up
  fclose(fp);

  // Cleanup libcurl
  curl_easy_cleanup(curl);

  // Cleanup libcurl global state
  curl_global_cleanup();
}
