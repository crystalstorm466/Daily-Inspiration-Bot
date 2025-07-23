#include <dpp/dpp.h>
#include <nlohmann/json.hpp>
#include <iostream>
#include <fstream>
#include <string>
#include "Scheduler.h"
#include <curl/curl.h>
#include <chrono>
#include <ctime>


static std::string BOT_TOKEN = std::getenv("DAILY_BOT_TOKEN");
const dpp::snowflake GENERAL_CHANNEL_ID = 870806579769905192;
const dpp::snowflake MONKI_CHANNEL_ID = 1393055197818916864;
const dpp::snowflake LUNCH_CHANNEL_ID = 1393059918499676220;
const dpp::snowflake DEV_CHANNEL_ID = 316679216881991682;

static bool AlreadyScheduled;
static std::string INSPIROBOT_API;

const  std::vector<dpp::snowflake> CHANNEL_IDS = {GENERAL_CHANNEL_ID, MONKI_CHANNEL_ID, LUNCH_CHANNEL_ID};
//const std::vector<dpp::snowflake> CHANNEL_IDS = {MONKI_CHANNEL_ID, DEV_CHANNEL_ID};
Bosma::Scheduler* global_bosma_scheduler = nullptr; //initialize
size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp) {
    ((std::string*)userp)->append((char*)contents, size * nmemb);
    return size * nmemb;
}

//fun to write received data directly to a file ( for image download)
size_t WriteImageCallback(void* contents, size_t size, size_t nmemb, void* userp) {
  std::ofstream* file = static_cast<std::ofstream*>(userp);

  size_t total_bytes = size * nmemb;

  if (total_bytes > 0) {
    file->write(static_cast<char*>(contents), total_bytes);
    if (file -> bad() || file->fail()) {
      std::cerr << "WriteImageCallback: Error writing to file!" << std::endl;
    }
  }
  return total_bytes;
}

void get_image(dpp::cluster& bot, bool isTest) {
  std::cout << "Attempting to fetch and send daily Inspirobot image..." << std::endl;
    CURL *curl;
    CURLcode res;
    std::string apiResponseBuffer;

    curl_global_init(CURL_GLOBAL_DEFAULT);
    curl = curl_easy_init();



  auto now = std::chrono::system_clock::now();
  std::time_t now_c = std::chrono::system_clock::to_time_t(now);
 
  
   if(curl) {
        std::cout << "Requesting image URL from inspirobot.." << std::endl;

        curl_easy_setopt(curl, CURLOPT_URL, "https://inspirobot.me/api?generate=true");
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &apiResponseBuffer);
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
        curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);

        res = curl_easy_perform(curl);

        if (res != CURLE_OK) {
          std::cerr << "curl_easy_perform() failed for URL reqest: " << curl_easy_strerror(res) << std::endl;
        } else {
          std::cout << "Recieved API response: " << apiResponseBuffer << std::endl;
          std::string imageUrl;

          try {
            //parse response
            nlohmann::json j = nlohmann::json::parse(apiResponseBuffer);
            if (j.is_string()) {
              imageUrl = j.get<std::string>();
            } else {
              std::cerr << "API response is not a simple string or expected JSON object." << std::endl;
            }

            } catch (const nlohmann::json::parse_error& e) {
              std::cerr << "JSON Parse error: " << e.what() << std::endl;
              if (apiResponseBuffer.rfind("http", 0) == 0) {
                imageUrl = apiResponseBuffer;
              }
            } catch ( const nlohmann::json::exception& e) {
              std::cerr << "JSON exception: " << e.what() << std::endl;
            }

            if (!imageUrl.empty() && imageUrl.rfind("http", 0) == 0) { // Final check if it looks like a URL
                std::cout << "Image URL extracted: " << imageUrl << std::endl;

                // --- STEP 2: Download the image from the extracted URL ---
                std::cout << "Downloading image..." << std::endl;
                std::ofstream outputFile("inspirobot_image.jpg", std::ios::binary);
                if (!outputFile.is_open()) {
                    std::cerr << "Failed to open file for writing: inspirobot_image.png" << std::endl;
                } else {
                    // Reset curl for the new request
                    curl_easy_reset(curl);
                    curl_easy_setopt(curl, CURLOPT_URL, imageUrl.c_str());
                    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteImageCallback);
                    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &outputFile);
                    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L); // WARNING: Disable SSL verification for testing.
                    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L); // Follow redirects
                    curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);
                    res = curl_easy_perform(curl);

                    if (res != CURLE_OK) {
                        std::cerr << "curl_easy_perform() failed for image download: " << curl_easy_strerror(res) << std::endl;
                    } else {
                        std::cout << "Image downloaded successfully to inspirobot_image.png" << std::endl;
                        outputFile.close();
                        // --- STEP 3: Send the downloaded image to Discord ---
                        //
                        for (int i = 0; i < CHANNEL_IDS.size(); i++ ) {
                          if (isTest) {
                            dpp::message downloaded_msg(CHANNEL_IDS[i], "Testing Daily Inspiration");
                            downloaded_msg.add_file("inspirobot_image.jpg", dpp::utility::read_file("inspirobot_image.jpg"));
                            bot.message_create(downloaded_msg);
                            continue;
                          }
                          dpp::message downloaded_msg(CHANNEL_IDS[i], "Daily Inspiration");
                          downloaded_msg.add_file("inspirobot_image.jpg", dpp::utility::read_file("inspirobot_image.jpg"));
                          bot.message_create(downloaded_msg);
                        }
                       
                    }
                    outputFile.close();
                }
            } else {
                std::cerr << "Could not extract a valid image URL from the API response." << std::endl;
            }
        } // End of if (res != CURLE_OK) for URL request

        curl_easy_cleanup(curl);
    } else {
        std::cerr << "Failed to initialize CURL." << std::endl;
    }
    curl_global_cleanup();
    return; 
    

}
void on_ready_handler(dpp::cluster &bot, const dpp::ready_t &event) {
    // This function is called when the bot is ready
    std::cout << "Bot is ready! Logged in as " << bot.me.username << std::endl;
    for (int i = 0; i < CHANNEL_IDS.size(); i++) {
      

     dpp::message msg (CHANNEL_IDS[i], "Bot is online. Daily Inspiration is scheduled");
     bot.message_create(msg);
     
    }
        get_image(bot, true);

       // Optionally, you can send a message to a specific channel
   // dpp::message msg (GENERAL_CHANNEL_ID, "");
   // msg.add_file("ben.jpg", dpp::utility::read_file("ben.jpg"));
  if (!AlreadyScheduled) {
    static Bosma::Scheduler main_scheduler(12);
    auto scheduler_error_callback = [](const std::exception &e) {
      std::cerr << "Bosma scheduler error: " << e.what() << std::endl;
    };

    global_bosma_scheduler = &main_scheduler;

    global_bosma_scheduler->cron("0 12 * * *", [&bot]() {
      get_image(bot, false);
    });
    AlreadyScheduled = true;

  } else {
    //.... continue

  }


    }
int main() {
    if (BOT_TOKEN.empty()) {
        std::cerr << "Error: BOT_TOKEN environment variable is not set." << std::endl;
        return EXIT_FAILURE;
    }
     AlreadyScheduled = false;
     dpp::cluster bot(BOT_TOKEN, dpp::i_default_intents | dpp::i_message_content);
  bot.on_log(dpp::utility::cout_logger());

  bot.on_ready([&bot](const dpp::ready_t &event) {
      std::cout << "Bot is ready! Logged in as " << bot.me.username << std::endl;
      on_ready_handler(bot, event);
  });
  bot.on_message_create([&bot](const dpp::message_create_t& event) {
      if (event.msg.content == "!test_daily") {
      get_image(bot, true);
      event.reply("Testing the bot...");
      }
      });

 try {
        bot.start(dpp::st_wait);
    } catch (const dpp::exception& e) {
        std::cerr << "DPP Exception: " << e.what() << std::endl;
        return 1;
    } catch (const std::exception& e) {
        std::cerr << "Standard Exception: " << e.what() << std::endl;
        return 1;
    } catch (...) {
        std::cerr << "Unknown exception caught in main." << std::endl;
        return 1;
    }
  return 0;
}
