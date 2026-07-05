#ifndef __IR_MENU_H__
#define __IR_MENU_H__

#include <IRac.h>
#include <IRremoteESP8266.h>
#include <MenuItemInterface.h>
#include <optional>

class IRMenu : public MenuItemInterface {
public:
    IRMenu() : MenuItemInterface("IR") {}
    ~IRMenu() {
        if (ac_controller) delete ac_controller;
    }

    void optionsMenu(void);
    void drawIcon(float scale);
    bool hasTheme() { return bruceConfig.theme.ir; }
    String themePath() { return bruceConfig.theme.paths.ir; }

private:
    enum class AcMenuScreen {
        Main,
        BrandSelect,
        Temperature,
        Mode,
        FanSpeed,
        Swing,
        Extras,
        Exit,
    };

    void configMenu(void);
    void StartAcBGone(void);
    void RunAcBGone(bool power_on);
    void AirConditionerMenu(void);
    void SelectAcBrand(void);
    void SendAcBGoneCommands(bool power_on);
    void SendAcStateChange(
        bool power, std::optional<stdAc::opmode_t> mode = std::nullopt,
        std::optional<float> temp = std::nullopt, std::optional<stdAc::fanspeed_t> fan = std::nullopt
    );
    void SetAcTemperature(void);
    void SetAcMode(void);
    void SetAcFanSpeed(void);
    void SetAcSwing(void);
    void AcExtrasMenu(void);
    void SendAcCurrentState(void);
    void ResetAcStateDefaults(void);
    void initializeAcState(decode_type_t protocol);
    bool isAcStatefulProtocol(decode_type_t protocol);
    String getAcProtocolLabel(decode_type_t protocol) const;

    void display_banner(void);

    // AC state management
    decode_type_t selected_ac_protocol = decode_type_t::UNKNOWN;
    stdAc::state_t current_ac_state;
    IRac *ac_controller = nullptr;
    AcMenuScreen ac_menu_screen = AcMenuScreen::Main;
    int ac_main_menu_index = 0;
    int ac_brand_menu_index = 0;
};

#endif
