
                                                                     // a broadcast scan is a scan which sends a message to all devices on the network to get their details
                                                                     // it will run quickly but may not find all devices
                                                                     // a direct scan is a scan which sends a message to a specific device to get its details
                                                                     // it will run slower but should find all devices
                                                                     // the program will first run a broadcast scan and then a direct scan (skipping the direct scans for devices already found by the broadcast scan)
                                                                     // this should in some cases help the program run quicker
                                                                     // the program will then report the final combined results sorted (as per the setting below)
																	 
                                                                     // once the combined results you will be able to scroll through the results on the TFT display by touching the screen
																	 
                                                                     // If you don't want to run a broadcast scan then set the broadcast scan timeout to 0
													 	 			                                                                       
#define SETTINGS_BROADCAST_SCAN_TIMEOUT                        3000  // number of milli-seconds to wait for response from a broadcast scan to find all devices
#define SETTINGS_DIRECT_SCAN_TIMEOUT                            750  // number of milli-seconds to wait for response from a direct scan to find one device
																	 
#define SETTINGS_FIRST_THREE_OCTETS_OF_SUBNET_TO_SCAN   "192.168.2"  // first three octets of network to scan  
#define SETTINGS_START_IP_HOST_OCTET                              1  // starting value for the host octet (should not be less than 1)
#define SETTINGS_END_IP_HOST_OCTET                              254  // ending value for the host octet (should not exceed 254; 255 is the broadcast address)
																	 
																	 
#define SETTINGS_SHOW_ALIAS                                    true  // all of these attributes will be printed to the Serial output
#define SETTINGS_SHOW_IP_ADDRESS                               true  // however only those that are set to true will be shown on the TFT display
#define SETTINGS_SHOW_MAC                                      true  //  
#define SETTINGS_SHOW_MODEL                                   false  //  
#define SETTINGS_SHOW_STATE                                    true  //  
																	 
#define SETTINGS_DELAY_BETWEEN_SCREENS_IN_MILLISECONDS         5000  // number of milli-seconds to wait between screen changes (to give the user time to review the TFT output)
                                                                     // if you are only interested in the final results, then this value may be set to 0 to allow the program to run quicker
																	 
#define SETTINGS_BROADCAST_SCAN_TFT_COLOUR                 TFT_BLUE  // colour of items reported by Broadcast scan
#define SETTINGS_DIRECT_SCAN_TFT_COLOUR                    TFT_CYAN  // colour of items reported by Direct scan
#define SETTINGS_COMBINED_RESULTS_TFT_COLOUR              TFT_GREEN  // colour of items reported by Combined results
#define SETTINGS_SHOW_TWO_COLOURS_IN_THE_DIRECT_SCAN          false  // set to true to show mixed colours in the direct scan report (broadcast scan results in one colour and direct scan results in another)
																	 
#define SETTINGS_SORT_BY_ALIAS                                 true  // set to true to sort the combined results by alias, set to false to sort the combined results by IP address
																	 
#define SETTINGS_TFT_WIDTH                                      240  // TFT Width in pixels
#define SETTINGS_TFT_HEIGHT                                     320  // TFT Height in pixels
																	 
#define SETTINGS_MAX_BUFFER_ROWS                                300  // max rows in the virtual window
#define SETTINGS_MAX_BUFFER_COLUMNS                             200  // max columns in the virtual window
