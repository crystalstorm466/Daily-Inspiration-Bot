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

//static  std::vector<dpp::snowflake> CHANNEL_IDS = {GENERAL_CHANNEL_ID, MONKI_CHANNEL_ID, LUNCH_CHANNEL_ID};
static std::vector<dpp::snowflake> CHANNEL_IDS = { 0 };
//const std::vector<dpp::snowflake> CHANNEL_IDS = {MONKI_CHANNEL_ID, DEV_CHANNEL_ID};
Bosma::Scheduler* global_bosma_scheduler = nullptr; //initialize
size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp) {
    ((std::string*)userp)->append((char*)contents, size * nmemb);
    return size * nmemb;
}

void readFile() {
  std::ifstream readChannels("subscribed_channels.txt");
  std::string line;

  if (readChannels.is_open() ) {
    CHANNEL_IDS.clear();
   while (std::getline(readChannels, line)) {
     if (!line.empty()) {
       try {
         dpp::snowflake id = std::stoull(line);
         CHANNEL_IDS.push_back(id);
       } catch (const std::invalid_argument& e) {
         std::cerr << "Invalid number format in subscribed_channels.txt: " << line << std::endl;
       } catch (const std::out_of_range& e) {
         std::cerr << "Number out of range in subscribed_channels.txt: " << line << std::endl;
       }
    }
  }
  readChannels.close();
} else {
  std::cerr << "Could not open 'subscribed_channels.txt' for reading" << std::endl;
}
}

void subscribeChannelID(dpp::cluster& bot, const dpp::message_create_t& event, dpp::snowflake channel_id) {

  for (const auto& id : CHANNEL_IDS) {
    if (id == channel_id) {
      event.reply("This channel is already subscribed.");
      return; //already subscribed
    }
  }
  std::ofstream outFile("subscribed_channels.txt", std::ios_base::app);
  if (outFile.is_open()) {
    outFile << channel_id << std::endl;
    outFile.close();
    CHANNEL_IDS.push_back(channel_id);
    return;
  } else {
    std::cerr << "Could not open subscribed_channels.txt for writing" << std::endl;
    return;
  }
}
void unSubscribeChannelID(dpp::cluster& bot, const dpp::message_create_t& event, dpp::snowflake channel_id) {
  std::vector<dpp::snowflake> temp_ids;


  std::ifstream inFile("subscribed_channels.txt");
  std::string line;
  bool was_found_in_file = false;

  if (inFile.is_open()) {
    while(std::getline(inFile, line)) {
      if (!line.empty()) {
        try {
          dpp::snowflake id = std::stoull(line);
          // Add all IDs *except* the one we want to unsubscribe
          if (id == channel_id) {
           was_found_in_file = true;
          } else {
            temp_ids.push_back(id);
          }
        } catch (const std::invalid_argument& e) {
           std::cerr << "Error: Invalid number format while reading for unsubscribe: '" << line << "' - " << e.what() << std::endl;
        } catch (const std::out_of_range& e) {
          std::cerr << "Error: Number out of range while reading for unsubscribe: '" << line << "' - " << e.what() << std::endl;
        }
      }
    }
    inFile.close();
  } else {
    std::cerr << "Error: Could not open 'subscribed_channels.txt' for reading during unsubscribe." << std::endl;
    // If we can't read the file, we can't safely unsubscribe.
    // You might want to reply to Discord here too.
    return;
  }
   if (!was_found_in_file) {
      std::cout << "Channel " << channel_id << " was not found in the subscription list. No action needed." << std::endl;
      return;
  }

  std::ofstream outFile("subscribed_channels.txt");
  if (outFile.is_open()) {
    for (const auto& id : temp_ids) {
      outFile << id << std::endl;
    }
    outFile.close();
    readFile();
    std::cout << "Successfully unsubscribed channel: " << channel_id << std::endl;
  } else {
     std::cerr << "Error: Could not open 'subscribed_channels.txt' for writing (rewrite step during unsubscribe)." << std::endl;
  }

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

void get_image(dpp::cluster& bot, bool isTest, dpp::snowflake channel_id) {
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
                        //
                            if (isTest) {

                            dpp::message downloaded_msg(channel_id, "Testing Daily Inspiration");
                            downloaded_msg.add_file("inspirobot_image.jpg", dpp::utility::read_file("inspirobot_image.jpg"));
                            bot.message_create(downloaded_msg);
                            return;
                          }
                          dpp::message downloaded_msg(channel_id, "Daily Inspiration");
                          downloaded_msg.add_file("inspirobot_image.jpg", dpp::utility::read_file("inspirobot_image.jpg"));
                          bot.message_create(downloaded_msg);
                        
                       
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

    dpp::message msg (1393059918499676220, "Bot is online. Daily Inspiration is scheduled");
     bot.message_create(msg);
      std::cout << "Online..." << std::endl;

        
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
        readFile(); //update the list
        for (int i = 0; i < CHANNEL_IDS.size(); i++ ) {
        get_image(bot, false, CHANNEL_IDS[i]);
        }
        
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
      readFile();
      on_ready_handler(bot, event);
  });
  bot.on_message_create([&bot](const dpp::message_create_t& event) {
      if (event.msg.content == "!test_daily") {
        readFile();
        get_image(bot, true, event.msg.channel_id);
        event.reply("Testing the bot...");
      }
      if (event.msg.content == "!subscribe") {
        //check if user or channel
        dpp::snowflake channel_id = event.msg.channel_id;
        subscribeChannelID(bot, event, channel_id);
        std::string reply_msg = "Channel `#";
        reply_msg += std::to_string(channel_id);

        reply_msg += ") has been added to Daily Inspiration List!";
        event.reply(reply_msg);
      }
      if (event.msg.content == "!unsubscribe") {
        dpp::snowflake channel_id = event.msg.channel_id;
        unSubscribeChannelID(bot, event, channel_id);
        std::string reply_msg = "Channel `#";
        reply_msg += std::to_string(channel_id);
        reply_msg += ") has been removed from Daily Inspiration List!";
        event.reply(reply_msg);;
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
