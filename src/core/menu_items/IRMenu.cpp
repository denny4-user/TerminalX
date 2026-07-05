#include "IRMenu.h"
#include "core/display.h"
#include "core/settings.h"
#include "core/utils.h"
#include "modules/ir/TV-B-Gone.h"
#include "modules/ir/custom_ir.h"
#include "modules/ir/ir_jammer.h"
#include "modules/ir/ir_read.h"
#include <IRac.h>
#include <IRremoteESP8266.h>
#include <algorithm>
#include <vector>

namespace {
using AcBrandEntry = std::pair<String, decode_type_t>;

std::vector<AcBrandEntry> getCompiledAcBrandEntries() {
    std::vector<AcBrandEntry> entries = {
#if SEND_DAIKIN
        {"Daikin",               decode_type_t::DAIKIN              },
#endif
#if SEND_DAIKIN2
        {"Daikin 2",             decode_type_t::DAIKIN2             },
#endif
#if SEND_DAIKIN128
        {"Daikin 128",           decode_type_t::DAIKIN128           },
#endif
#if SEND_DAIKIN152
        {"Daikin 152",           decode_type_t::DAIKIN152           },
#endif
#if SEND_DAIKIN160
        {"Daikin 160",           decode_type_t::DAIKIN160           },
#endif
#if SEND_DAIKIN176
        {"Daikin 176",           decode_type_t::DAIKIN176           },
#endif
#if SEND_DAIKIN216
        {"Daikin 216",           decode_type_t::DAIKIN216           },
#endif
#if SEND_DAIKIN64
        {"Daikin 64",            decode_type_t::DAIKIN64            },
#endif
#if SEND_MITSUBISHI_AC
        {"Mitsubishi",           decode_type_t::MITSUBISHI_AC       },
#endif
#if SEND_MITSUBISHI112
        {"Mitsubishi 112",       decode_type_t::MITSUBISHI112       },
#endif
#if SEND_MITSUBISHI136
        {"Mitsubishi 136",       decode_type_t::MITSUBISHI136       },
#endif
#if SEND_MITSUBISHIHEAVY
        {"Mitsubishi Heavy 88",  decode_type_t::MITSUBISHI_HEAVY_88 },
        {"Mitsubishi Heavy 152", decode_type_t::MITSUBISHI_HEAVY_152},
#endif
#if SEND_PANASONIC_AC
        {"Panasonic",            decode_type_t::PANASONIC_AC        },
#endif
#if SEND_PANASONIC_AC32
        {"Panasonic 32",         decode_type_t::PANASONIC_AC32      },
#endif
#if SEND_SAMSUNG_AC
        {"Samsung",              decode_type_t::SAMSUNG_AC          },
#endif
#if SEND_LG
        {"LG",                   decode_type_t::LG                  },
#endif
#if SEND_SHARP_AC
        {"Sharp",                decode_type_t::SHARP_AC            },
#endif
#if SEND_TOSHIBA_AC
        {"Toshiba",              decode_type_t::TOSHIBA_AC          },
#endif
#if SEND_COOLIX
        {"Coolix",               decode_type_t::COOLIX              },
#endif
#if SEND_KELVINATOR
        {"Kelvinator",           decode_type_t::KELVINATOR          },
#endif
#if SEND_HAIER_AC
        {"Haier",                decode_type_t::HAIER_AC            },
#endif
#if SEND_HAIER_AC160
        {"Haier 160",            decode_type_t::HAIER_AC160         },
#endif
#if SEND_HAIER_AC176
        {"Haier 176",            decode_type_t::HAIER_AC176         },
#endif
#if SEND_HAIER_AC_YRW02
        {"Haier YRW02",          decode_type_t::HAIER_AC_YRW02      },
#endif
#if SEND_HITACHI_AC
        {"Hitachi",              decode_type_t::HITACHI_AC          },
#endif
#if SEND_HITACHI_AC1
        {"Hitachi 1",            decode_type_t::HITACHI_AC1         },
#endif
#if SEND_HITACHI_AC264
        {"Hitachi 264",          decode_type_t::HITACHI_AC264       },
#endif
#if SEND_HITACHI_AC296
        {"Hitachi 296",          decode_type_t::HITACHI_AC296       },
#endif
#if SEND_HITACHI_AC344
        {"Hitachi 344",          decode_type_t::HITACHI_AC344       },
#endif
#if SEND_HITACHI_AC424
        {"Hitachi 424",          decode_type_t::HITACHI_AC424       },
#endif
#if SEND_WHIRLPOOL_AC
        {"Whirlpool",            decode_type_t::WHIRLPOOL_AC        },
#endif
#if SEND_VOLTAS
        {"Voltas",               decode_type_t::VOLTAS              },
#endif
#if SEND_YORK
        {"York",                 decode_type_t::YORK                },
#endif
#if SEND_FUJITSU_AC
        {"Fujitsu",              decode_type_t::FUJITSU_AC          },
#endif
#if SEND_GREE
        {"Gree",                 decode_type_t::GREE                },
#endif
#if SEND_MIDEA
        {"Midea",                decode_type_t::MIDEA               },
#endif
#if SEND_TCL112AC
        {"TCL",                  decode_type_t::TCL112AC            },
#endif
#if SEND_TROTEC
        {"Trotec",               decode_type_t::TROTEC              },
#endif
#if SEND_TROTEC_3550
        {"Trotec 3550",          decode_type_t::TROTEC_3550         },
#endif
#if SEND_VESTEL_AC
        {"Vestel",               decode_type_t::VESTEL_AC           },
#endif
#if SEND_TECHNIBEL_AC
        {"Technibel",            decode_type_t::TECHNIBEL_AC        },
#endif
#if SEND_TECO
        {"Teco",                 decode_type_t::TECO                },
#endif
#if SEND_SANYO_AC
        {"Sanyo",                decode_type_t::SANYO_AC            },
#endif
#if SEND_SANYO_AC88
        {"Sanyo 88",             decode_type_t::SANYO_AC88          },
#endif
#if SEND_NEOCLIMA
        {"Neoclima",             decode_type_t::NEOCLIMA            },
#endif
#if SEND_GOODWEATHER
        {"Goodweather",          decode_type_t::GOODWEATHER         },
#endif
#if SEND_ELECTRA_AC
        {"Electra",              decode_type_t::ELECTRA_AC          },
#endif
#if SEND_DELONGHI_AC
        {"Delonghi",             decode_type_t::DELONGHI_AC         },
#endif
#if SEND_ECOCLIM
        {"Ecoclim",              decode_type_t::ECOCLIM             },
#endif
#if SEND_CORONA_AC
        {"Corona",               decode_type_t::CORONA_AC           },
#endif
#if SEND_RHOSS
        {"Rhoss",                decode_type_t::RHOSS               },
#endif
#if SEND_KELON
        {"Kelon",                decode_type_t::KELON               },
#endif
#if SEND_TRANSCOLD
        {"Transcold",            decode_type_t::TRANSCOLD           },
#endif
#if SEND_TRUMA
        {"Truma",                decode_type_t::TRUMA               },
#endif
#if SEND_ARGO
        {"Argo",                 decode_type_t::ARGO                },
#endif
#if SEND_BOSCH144
        {"Bosch 144",            decode_type_t::BOSCH144            },
#endif
    };

    return entries;
}

std::vector<AcBrandEntry> getSupportedAcBrandEntries() {
    std::vector<AcBrandEntry> supported_entries;

    for (const auto &entry : getCompiledAcBrandEntries()) {
        if (entry.second != decode_type_t::UNKNOWN && IRac::isProtocolSupported(entry.second) &&
            hasACState(entry.second)) {
            supported_entries.push_back(entry);
        }
    }

    return supported_entries;
}
} // namespace

void IRMenu::optionsMenu() {
#if defined(ARDUINO_M5STICK_S3)
    bool prevPower = M5.Power.getExtOutput();
    M5.Power.setExtOutput(true); // ENABLE 5V OUTPUT
#endif
    options = {
        {"AC-B-Gone",       [this]() { StartAcBGone(); }      },
        {"TV-B-Gone",       StartTvBGone                      },
        {"Air Conditioner", [this]() { AirConditionerMenu(); }},
        {"Custom IR",       otherIRcodes                      },
        {"IR Read",         [=]() { IrRead(); }               },
#if !defined(LITE_VERSION)
        {"IR Jammer",       startIrJammer                     }, // Simple frequency-adjustable jammer
#endif
        {"Config",          [this]() { configMenu(); }        },
    };
    addOptionToMainMenu();

    String txt = "Infrared";
    txt += " Tx: " + String(bruceConfigPins.irTx) + " Rx: " + String(bruceConfigPins.irRx) +
           " Rpts: " + String(bruceConfigPins.irTxRepeats);
    loopOptions(options, MENU_TYPE_SUBMENU, txt.c_str());
#if defined(ARDUINO_M5STICK_S3)
    M5.Power.setExtOutput(prevPower);
#endif
}

void IRMenu::configMenu() {
    options = {
        {"Ir TX Pin", lambdaHelper(gsetIrTxPin, true)},
        {"Ir RX Pin", lambdaHelper(gsetIrRxPin, true)},
        {"Ir TX Repeats", setIrTxRepeats},
        {"Back", [this]() { optionsMenu(); }},
    };

    loopOptions(options, MENU_TYPE_SUBMENU, "IR Config");
}

void IRMenu::StartAcBGone() {
    display_banner();
    tft.setTextSize(FM);
    tft.println("AC-B-Gone");
    tft.println("");

    options = {
        {"OFF",           [this]() { RunAcBGone(false); }},
        {"ON Full Blast", [this]() { RunAcBGone(true); } },
        {"Back",          [this]() { optionsMenu(); }    },
    };

    loopOptions(options, MENU_TYPE_SUBMENU, "AC-B-Gone");
}

void IRMenu::RunAcBGone(bool power_on) {
    checkIrTxPin();
    display_banner();
    tft.setTextSize(FM);
    tft.println("AC-B-Gone");
    tft.println(power_on ? "ON Full Blast" : "OFF");
    tft.println("all supported AC protocols...");
    delay(1000);

    SendAcBGoneCommands(power_on);

    tft.println("AC-B-Gone complete!");
    delay(2000);
}
void IRMenu::display_banner() {
    drawMainBorderWithTitle("AC Control");
    padprintln("\n");
}
void IRMenu::AirConditionerMenu() {
    checkIrTxPin();

    if (selected_ac_protocol == decode_type_t::UNKNOWN || !isAcStatefulProtocol(selected_ac_protocol)) {
        selected_ac_protocol = decode_type_t::UNKNOWN;
        ac_menu_screen = AcMenuScreen::BrandSelect;
    } else {
        ac_menu_screen = AcMenuScreen::Main;
    }

    while (ac_menu_screen != AcMenuScreen::Exit) {
        switch (ac_menu_screen) {
            case AcMenuScreen::BrandSelect: SelectAcBrand(); break;
            case AcMenuScreen::Temperature: SetAcTemperature(); break;
            case AcMenuScreen::Mode: SetAcMode(); break;
            case AcMenuScreen::FanSpeed: SetAcFanSpeed(); break;
            case AcMenuScreen::Swing: SetAcSwing(); break;
            case AcMenuScreen::Extras: AcExtrasMenu(); break;
            case AcMenuScreen::Main: {
                display_banner();
                tft.setTextSize(FM);
                tft.printf("Protocol: %s\n", getAcProtocolLabel(selected_ac_protocol).c_str());
                tft.printf("Power: %s\n", current_ac_state.power ? "ON" : "OFF");
                if (current_ac_state.power) {
                    tft.printf(
                        "Mode: %s\n",
                        current_ac_state.mode == stdAc::opmode_t::kCool   ? "Cool"
                        : current_ac_state.mode == stdAc::opmode_t::kHeat ? "Heat"
                        : current_ac_state.mode == stdAc::opmode_t::kAuto ? "Auto"
                        : current_ac_state.mode == stdAc::opmode_t::kDry  ? "Dry"
                        : current_ac_state.mode == stdAc::opmode_t::kFan  ? "Fan"
                                                                          : "Unknown"
                    );
                    tft.printf("Temp: %.1f C\n", current_ac_state.degrees);
                    tft.printf(
                        "Fan: %s\n",
                        current_ac_state.fanspeed == stdAc::fanspeed_t::kAuto     ? "Auto"
                        : current_ac_state.fanspeed == stdAc::fanspeed_t::kMin    ? "Min"
                        : current_ac_state.fanspeed == stdAc::fanspeed_t::kLow    ? "Low"
                        : current_ac_state.fanspeed == stdAc::fanspeed_t::kMedium ? "Med"
                        : current_ac_state.fanspeed == stdAc::fanspeed_t::kHigh   ? "High"
                        : current_ac_state.fanspeed == stdAc::fanspeed_t::kMax    ? "Max"
                                                                                  : "Unknown"
                    );
                }
                tft.println("");

                options = {
                    {"Power Toggle",
                     [this]() {
                         SendAcStateChange(!current_ac_state.power);
                         ac_menu_screen = AcMenuScreen::Main;
                     }                                                                       },
                    {"Temp Up",
                     [this]() {
                         SendAcStateChange(true, std::nullopt, current_ac_state.degrees + 1.0f);
                         ac_menu_screen = AcMenuScreen::Main;
                     }                                                                       },
                    {"Temp Down",
                     [this]() {
                         SendAcStateChange(true, std::nullopt, current_ac_state.degrees - 1.0f);
                         ac_menu_screen = AcMenuScreen::Main;
                     }                                                                       },
                    {"Mode: Cool",
                     [this]() {
                         SendAcStateChange(true, stdAc::opmode_t::kCool);
                         ac_menu_screen = AcMenuScreen::Main;
                     }                                                                       },
                    {"Mode: Heat",
                     [this]() {
                         SendAcStateChange(true, stdAc::opmode_t::kHeat);
                         ac_menu_screen = AcMenuScreen::Main;
                     }                                                                       },
                    {"Mode: Auto",
                     [this]() {
                         SendAcStateChange(true, stdAc::opmode_t::kAuto);
                         ac_menu_screen = AcMenuScreen::Main;
                     }                                                                       },
                    {"Fan: Auto",
                     [this]() {
                         SendAcStateChange(true, std::nullopt, std::nullopt, stdAc::fanspeed_t::kAuto);
                         ac_menu_screen = AcMenuScreen::Main;
                     }                                                                       },
                    {"Fan: Low",
                     [this]() {
                         SendAcStateChange(true, std::nullopt, std::nullopt, stdAc::fanspeed_t::kLow);
                         ac_menu_screen = AcMenuScreen::Main;
                     }                                                                       },
                    {"Fan: Med",
                     [this]() {
                         SendAcStateChange(true, std::nullopt, std::nullopt, stdAc::fanspeed_t::kMedium);
                         ac_menu_screen = AcMenuScreen::Main;
                     }                                                                       },
                    {"Fan: High",
                     [this]() {
                         SendAcStateChange(true, std::nullopt, std::nullopt, stdAc::fanspeed_t::kHigh);
                         ac_menu_screen = AcMenuScreen::Main;
                     }                                                                       },
                    {"Set Temp",     [this]() { ac_menu_screen = AcMenuScreen::Temperature; }},
                    {"Mode",         [this]() { ac_menu_screen = AcMenuScreen::Mode; }       },
                    {"Fan Speed",    [this]() { ac_menu_screen = AcMenuScreen::FanSpeed; }   },
                    {"Swing",        [this]() { ac_menu_screen = AcMenuScreen::Swing; }      },
                    {"Extras",       [this]() { ac_menu_screen = AcMenuScreen::Extras; }     },
                    {"Change Brand",
                     [this]() {
                         selected_ac_protocol = decode_type_t::UNKNOWN;
                         ac_menu_screen = AcMenuScreen::BrandSelect;
                     }                                                                       },
                    {"Back",         [this]() { ac_menu_screen = AcMenuScreen::Exit; }       },
                };

                if (ac_main_menu_index < 0) ac_main_menu_index = 0;
                if (ac_main_menu_index >= static_cast<int>(options.size())) {
                    ac_main_menu_index = options.size() - 1;
                }

                int result = loopOptions(options, MENU_TYPE_SUBMENU, "Air Conditioner", ac_main_menu_index);
                if (result < 0) {
                    ac_menu_screen = AcMenuScreen::Exit;
                } else {
                    ac_main_menu_index = result;
                }
                break;
            }
            case AcMenuScreen::Exit: break;
        }
    }
}

void IRMenu::SendAcBGoneCommands(bool power_on) {
    IRac ac(bruceConfigPins.irTx);
    std::vector<AcBrandEntry> ac_brands = getSupportedAcBrandEntries();
    const unsigned long total = static_cast<unsigned long>(ac_brands.size());

    tft.setTextSize(FM);
    tft.printf("Sending %s to %lu AC protocols...\n", power_on ? "ON" : "OFF", total);

    unsigned long current = 0;
    for (const auto &brand : ac_brands) {
        const decode_type_t protocol = brand.second;

        current++;
        display_banner();
        tft.setTextSize(FM);
        tft.println("AC-B-Gone");
        tft.printf("%s\n", power_on ? "ON Full Blast" : "OFF");
        tft.printf("Brand: %s\n", brand.first.c_str());
        tft.printf("Protocol: %s\n", getAcProtocolLabel(protocol).c_str());
        tft.printf("%lu/%lu\n", current, total);

        stdAc::state_t state;
        IRac::initState(
            &state,
            protocol,
            -1,
            power_on,
            power_on ? stdAc::opmode_t::kCool : stdAc::opmode_t::kOff,
            power_on ? 16.0f : 25.0f,
            true,
            power_on ? stdAc::fanspeed_t::kMax : stdAc::fanspeed_t::kAuto,
            power_on ? stdAc::swingv_t::kAuto : stdAc::swingv_t::kOff,
            power_on ? stdAc::swingh_t::kAuto : stdAc::swingh_t::kOff,
            false,
            power_on,
            false,
            false,
            false,
            false,
            false,
            -1,
            -1
        );
        state.command = stdAc::ac_command_t::kControlCommand;

        ac.sendAc(state);
        delay(350);
    }

    tft.println("AC-B-Gone complete!");
}

bool IRMenu::isAcStatefulProtocol(decode_type_t protocol) {
    return protocol != decode_type_t::UNKNOWN && IRac::isProtocolSupported(protocol) && hasACState(protocol);
}

String IRMenu::getAcProtocolLabel(decode_type_t protocol) const {
    for (const auto &brand : getCompiledAcBrandEntries()) {
        if (brand.second == protocol) return brand.first;
    }

    return typeToString(protocol);
}

void IRMenu::initializeAcState(decode_type_t protocol) {
    current_ac_state.protocol = protocol;
    current_ac_state.model = -1;
    current_ac_state.power = false;
    current_ac_state.mode = stdAc::opmode_t::kCool;
    current_ac_state.degrees = 24.0f;
    current_ac_state.celsius = true;
    current_ac_state.fanspeed = stdAc::fanspeed_t::kAuto;
    current_ac_state.swingv = stdAc::swingv_t::kOff;
    current_ac_state.swingh = stdAc::swingh_t::kOff;
    current_ac_state.quiet = false;
    current_ac_state.turbo = false;
    current_ac_state.econo = false;
    current_ac_state.light = false;
    current_ac_state.filter = false;
    current_ac_state.clean = false;
    current_ac_state.beep = false;
    current_ac_state.sleep = -1;
    current_ac_state.clock = -1;
    current_ac_state.command = stdAc::ac_command_t::kControlCommand;
    current_ac_state.iFeel = false;
    current_ac_state.sensorTemperature = kNoTempValue;
}

void IRMenu::SendAcCurrentState() {
    if (!ac_controller) { ac_controller = new IRac(bruceConfigPins.irTx); }

    display_banner();
    tft.setTextSize(FM);
    tft.println("Sending current AC state...");
    tft.printf("Power: %s\n", current_ac_state.power ? "ON" : "OFF");
    ac_controller->sendAc(current_ac_state);
    tft.println("Command sent!");
    delay(250);
}

void IRMenu::ResetAcStateDefaults() {
    initializeAcState(selected_ac_protocol);
    display_banner();
    tft.setTextSize(FM);
    tft.println("Resetting AC defaults...");
    SendAcCurrentState();
}

void IRMenu::SetAcTemperature() {
    display_banner();
    tft.setTextSize(FM);
    tft.println("Select Temperature:");
    tft.println("");

    options.clear();
    for (int t = 16; t <= 30; t += 1) {
        options.push_back({String(t) + " C", [this, t]() {
                               SendAcStateChange(true, std::nullopt, static_cast<float>(t));
                               ac_menu_screen = AcMenuScreen::Main;
                           }});
    }
    options.push_back({"Back", [this]() { ac_menu_screen = AcMenuScreen::Main; }});
    int result = loopOptions(options, MENU_TYPE_SUBMENU, "Set Temperature");
    if (result < 0) ac_menu_screen = AcMenuScreen::Main;
}

void IRMenu::SetAcMode() {
    display_banner();
    tft.setTextSize(FM);
    tft.println("Select Mode:");
    tft.println("");

    options = {
        {"Auto",
         [this]() {
             SendAcStateChange(true, stdAc::opmode_t::kAuto);
             ac_menu_screen = AcMenuScreen::Main;
         }                                                        },
        {"Cool",
         [this]() {
             SendAcStateChange(true, stdAc::opmode_t::kCool);
             ac_menu_screen = AcMenuScreen::Main;
         }                                                        },
        {"Heat",
         [this]() {
             SendAcStateChange(true, stdAc::opmode_t::kHeat);
             ac_menu_screen = AcMenuScreen::Main;
         }                                                        },
        {"Dry",
         [this]() {
             SendAcStateChange(true, stdAc::opmode_t::kDry);
             ac_menu_screen = AcMenuScreen::Main;
         }                                                        },
        {"Fan",
         [this]() {
             SendAcStateChange(true, stdAc::opmode_t::kFan);
             ac_menu_screen = AcMenuScreen::Main;
         }                                                        },
        {"Back", [this]() { ac_menu_screen = AcMenuScreen::Main; }},
    };
    int result = loopOptions(options, MENU_TYPE_SUBMENU, "Select Mode");
    if (result < 0) ac_menu_screen = AcMenuScreen::Main;
}

void IRMenu::SetAcFanSpeed() {
    display_banner();
    tft.setTextSize(FM);
    tft.println("Select Fan Speed:");
    tft.println("");

    options = {
        {"Auto",
         [this]() {
             SendAcStateChange(true, std::nullopt, std::nullopt, stdAc::fanspeed_t::kAuto);
             ac_menu_screen = AcMenuScreen::Main;
         }                                                        },
        {"Min",
         [this]() {
             SendAcStateChange(true, std::nullopt, std::nullopt, stdAc::fanspeed_t::kMin);
             ac_menu_screen = AcMenuScreen::Main;
         }                                                        },
        {"Low",
         [this]() {
             SendAcStateChange(true, std::nullopt, std::nullopt, stdAc::fanspeed_t::kLow);
             ac_menu_screen = AcMenuScreen::Main;
         }                                                        },
        {"Med",
         [this]() {
             SendAcStateChange(true, std::nullopt, std::nullopt, stdAc::fanspeed_t::kMedium);
             ac_menu_screen = AcMenuScreen::Main;
         }                                                        },
        {"High",
         [this]() {
             SendAcStateChange(true, std::nullopt, std::nullopt, stdAc::fanspeed_t::kHigh);
             ac_menu_screen = AcMenuScreen::Main;
         }                                                        },
        {"Max",
         [this]() {
             SendAcStateChange(true, std::nullopt, std::nullopt, stdAc::fanspeed_t::kMax);
             ac_menu_screen = AcMenuScreen::Main;
         }                                                        },
        {"Back", [this]() { ac_menu_screen = AcMenuScreen::Main; }},
    };
    int result = loopOptions(options, MENU_TYPE_SUBMENU, "Fan Speed");
    if (result < 0) ac_menu_screen = AcMenuScreen::Main;
}

void IRMenu::SetAcSwing() {
    display_banner();
    tft.setTextSize(FM);
    tft.println("Swing settings:");
    tft.println("");

    options = {
        {"V Off",
         [this]() {
             current_ac_state.swingv = stdAc::swingv_t::kOff;
             SendAcCurrentState();
             ac_menu_screen = AcMenuScreen::Main;
         }                                                          },
        {"V Auto",
         [this]() {
             current_ac_state.swingv = stdAc::swingv_t::kAuto;
             SendAcCurrentState();
             ac_menu_screen = AcMenuScreen::Main;
         }                                                          },
        {"V High",
         [this]() {
             current_ac_state.swingv = stdAc::swingv_t::kHigh;
             SendAcCurrentState();
             ac_menu_screen = AcMenuScreen::Main;
         }                                                          },
        {"V Mid",
         [this]() {
             current_ac_state.swingv = stdAc::swingv_t::kMiddle;
             SendAcCurrentState();
             ac_menu_screen = AcMenuScreen::Main;
         }                                                          },
        {"V Low",
         [this]() {
             current_ac_state.swingv = stdAc::swingv_t::kLow;
             SendAcCurrentState();
             ac_menu_screen = AcMenuScreen::Main;
         }                                                          },
        {"H Off",
         [this]() {
             current_ac_state.swingh = stdAc::swingh_t::kOff;
             SendAcCurrentState();
             ac_menu_screen = AcMenuScreen::Main;
         }                                                          },
        {"H Auto",
         [this]() {
             current_ac_state.swingh = stdAc::swingh_t::kAuto;
             SendAcCurrentState();
             ac_menu_screen = AcMenuScreen::Main;
         }                                                          },
        {"Back",   [this]() { ac_menu_screen = AcMenuScreen::Main; }},
    };
    int result = loopOptions(options, MENU_TYPE_SUBMENU, "Swing");
    if (result < 0) ac_menu_screen = AcMenuScreen::Main;
}

void IRMenu::AcExtrasMenu() {
    display_banner();
    tft.setTextSize(FM);
    tft.println("Extras:");
    tft.println("");

    options = {
        {"Turbo",
         [this]() {
             current_ac_state.turbo = !current_ac_state.turbo;
             SendAcCurrentState();
             ac_menu_screen = AcMenuScreen::Main;
         }                                                                      },
        {"Light",
         [this]() {
             current_ac_state.light = !current_ac_state.light;
             SendAcCurrentState();
             ac_menu_screen = AcMenuScreen::Main;
         }                                                                      },
        {"Econo",
         [this]() {
             current_ac_state.econo = !current_ac_state.econo;
             SendAcCurrentState();
             ac_menu_screen = AcMenuScreen::Main;
         }                                                                      },
        {"Sleep 30m",
         [this]() {
             current_ac_state.sleep = (current_ac_state.sleep == 30) ? -1 : 30;
             SendAcCurrentState();
             ac_menu_screen = AcMenuScreen::Main;
         }                                                                      },
        {"Send Current State",
         [this]() {
             SendAcCurrentState();
             ac_menu_screen = AcMenuScreen::Main;
         }                                                                      },
        {"Reset Defaults",
         [this]() {
             ResetAcStateDefaults();
             ac_menu_screen = AcMenuScreen::Main;
         }                                                                      },
        {"Back",               [this]() { ac_menu_screen = AcMenuScreen::Main; }},
    };
    int result = loopOptions(options, MENU_TYPE_SUBMENU, "AC Extras");
    if (result < 0) ac_menu_screen = AcMenuScreen::Main;
}

void IRMenu::SelectAcBrand() {
    display_banner();
    tft.setTextSize(FM);
    tft.println("Select AC Brand/Protocol:");
    tft.println("");

    options.clear();
    std::vector<AcBrandEntry> ac_brands = getSupportedAcBrandEntries();

    for (const auto &brand : ac_brands) {
        options.push_back({brand.first.c_str(), [this, brand]() {
                               selected_ac_protocol = brand.second;
                               initializeAcState(brand.second);
                               ac_menu_screen = AcMenuScreen::Main;
                           }});
    }

    options.push_back({"Back", [this]() { ac_menu_screen = AcMenuScreen::Exit; }});

    if (options.size() == 1) {
        tft.println("\nNo stateful AC protocols compiled.");
        delay(2000);
        ac_menu_screen = AcMenuScreen::Exit;
        return;
    }

    if (ac_brand_menu_index < 0) ac_brand_menu_index = 0;
    if (ac_brand_menu_index >= static_cast<int>(options.size())) {
        ac_brand_menu_index = options.size() - 1;
    }

    int result = loopOptions(options, MENU_TYPE_SUBMENU, "Select AC Brand", ac_brand_menu_index);
    if (result < 0) {
        ac_menu_screen = AcMenuScreen::Exit;
    } else {
        ac_brand_menu_index = result;
    }
}

void IRMenu::SendAcStateChange(
    bool power, std::optional<stdAc::opmode_t> mode, std::optional<float> temp,
    std::optional<stdAc::fanspeed_t> fan
) {
    current_ac_state.power = power;

    if (mode.has_value()) { current_ac_state.mode = *mode; }

    if (temp.has_value()) { current_ac_state.degrees = std::min(30.0f, std::max(16.0f, *temp)); }

    if (fan.has_value()) { current_ac_state.fanspeed = *fan; }

    if (!ac_controller) { ac_controller = new IRac(bruceConfigPins.irTx); }

    display_banner();
    tft.setTextSize(FM);
    tft.printf("Sending AC command...\n");
    tft.printf("Protocol: %s\n", getAcProtocolLabel(selected_ac_protocol).c_str());
    tft.printf("Power: %s\n", current_ac_state.power ? "ON" : "OFF");
    if (current_ac_state.power) {
        tft.printf(
            "Mode: %s\n",
            current_ac_state.mode == stdAc::opmode_t::kCool   ? "Cool"
            : current_ac_state.mode == stdAc::opmode_t::kHeat ? "Heat"
            : current_ac_state.mode == stdAc::opmode_t::kAuto ? "Auto"
            : current_ac_state.mode == stdAc::opmode_t::kDry  ? "Dry"
            : current_ac_state.mode == stdAc::opmode_t::kFan  ? "Fan"
                                                              : "Unknown"
        );
        tft.printf("Temp: %.1f°C\n", current_ac_state.degrees);
        tft.printf(
            "Fan: %s\n",
            current_ac_state.fanspeed == stdAc::fanspeed_t::kAuto     ? "Auto"
            : current_ac_state.fanspeed == stdAc::fanspeed_t::kMin    ? "Min"
            : current_ac_state.fanspeed == stdAc::fanspeed_t::kLow    ? "Low"
            : current_ac_state.fanspeed == stdAc::fanspeed_t::kMedium ? "Med"
            : current_ac_state.fanspeed == stdAc::fanspeed_t::kHigh   ? "High"
            : current_ac_state.fanspeed == stdAc::fanspeed_t::kMax    ? "Max"
                                                                      : "Unknown"
        );
    }

    ac_controller->sendAc(current_ac_state);
    tft.println("Command sent!");
    delay(250);
}

void IRMenu::drawIcon(float scale) {
    clearIconArea();
    int iconSize = scale * 60;
    int radius = scale * 7;
    int deltaRadius = scale * 10;

    if (iconSize % 2 != 0) iconSize++;

    tft.fillRect(
        iconCenterX - iconSize / 2, iconCenterY - iconSize / 2, iconSize / 6, iconSize, bruceConfig.priColor
    );
    tft.fillRect(
        iconCenterX - iconSize / 3,
        iconCenterY - iconSize / 3,
        iconSize / 6,
        2 * iconSize / 3,
        bruceConfig.priColor
    );

    tft.drawCircle(iconCenterX - iconSize / 6, iconCenterY, radius, bruceConfig.priColor);

    tft.drawArc(
        iconCenterX - iconSize / 6,
        iconCenterY,
        2.5 * radius,
        2 * radius,
        220,
        320,
        bruceConfig.priColor,
        bruceConfig.bgColor
    );
    tft.drawArc(
        iconCenterX - iconSize / 6,
        iconCenterY,
        2.5 * radius + deltaRadius,
        2 * radius + deltaRadius,
        220,
        320,
        bruceConfig.priColor,
        bruceConfig.bgColor
    );
    tft.drawArc(
        iconCenterX - iconSize / 6,
        iconCenterY,
        2.5 * radius + 2 * deltaRadius,
        2 * radius + 2 * deltaRadius,
        220,
        320,
        bruceConfig.priColor,
        bruceConfig.bgColor
    );
}
