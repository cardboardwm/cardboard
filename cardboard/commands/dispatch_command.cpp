#include "../Command.h"
#include "commands.h"

#include <numeric>
#include <variant>

template <class... Ts>
struct overloaded : Ts... {
    using Ts::operator()...;
};
template <class... Ts>
overloaded(Ts...) -> overloaded<Ts...>;

uint32_t modifier_array_to_mask(const std::vector<std::string>& modifiers)
{
    static const std::unordered_map<std::string, uint32_t> mod_table = {
        { "shift", WLR_MODIFIER_SHIFT },
        { "ctrl", WLR_MODIFIER_CTRL },
        { "alt", WLR_MODIFIER_ALT },
        { "super", WLR_MODIFIER_LOGO },
        { "caps", WLR_MODIFIER_CAPS },
        { "mod2", WLR_MODIFIER_MOD2 },
        { "mod3", WLR_MODIFIER_MOD3 },
        { "mod5", WLR_MODIFIER_MOD5 }
    };

    uint32_t mask = 0;
    for (auto mod : modifiers) {
        mask |= mod_table.at(mod);
    }
    return mask;
}

static Command dispatch_workspace(const command_arguments::workspace& workspace)
{
    return std::visit(overloaded {
                          [](command_arguments::workspace::switch_ switch_) -> Command {
                              return [switch_](Server* server) {
                                  return commands::workspace_switch(server, switch_.n);
                              };
                          },
                          [](command_arguments::workspace::move move) -> Command {
                              return [move](Server* server) {
                                  return commands::workspace_move(server, move.n);
                              };
                          },
                      },
                      workspace.workspace);
}

Command dispatch_command(const CommandData& command_data)
{
    return std::visit(overloaded {
                          [](const command_arguments::focus focus_data) -> Command {
                              if (!focus_data.cycle) {
                                  return [focus_data](Server* server) {
                                      return commands::focus(
                                          server,
                                          focus_data.direction == command_arguments::focus::Direction::Left ? -1 : +1);
                                  };
                              } else {
                                  return commands::focus_cycle;
                              }
                          },
                          [](const command_arguments::quit quit_data) -> Command {
                              return [quit_data](Server* server) { return commands::quit(server, quit_data.code); };
                          },
                          [](const command_arguments::bind& bind_data) -> Command {
                              uint32_t modifiers = modifier_array_to_mask(bind_data.modifiers);

                              xkb_keysym_t sym = xkb_keysym_from_name(bind_data.key.c_str(), XKB_KEYSYM_CASE_INSENSITIVE);

                              if (sym == XKB_KEY_NoSymbol)
                                  return { [bind_data](Server*) -> CommandResult { return { std::string("Invalid keysym: ") + bind_data.key }; } };

                              Command command = dispatch_command(*(bind_data.command));
                              return { [modifiers, sym, command](Server* server) {
                                  return commands::bind(server, modifiers, sym, command);
                              } };
                          },
                          [](const command_arguments::exec exec_data) -> Command {
                              return [exec_data](Server* server) {
                                  return commands::exec(server, exec_data.argv);
                              };
                          },
                          [](const command_arguments::close) -> Command {
                              return commands::close;
                          },
                          [](const command_arguments::workspace& workspace) -> Command {
                              return dispatch_workspace(workspace);
                          },
                          [](const command_arguments::toggle_floating&) -> Command {
                              return commands::toggle_floating;
                          },
                          [](const command_arguments::move& move) -> Command {
                              return [move](Server* server) {
                                  return commands::move(server, move.dx, move.dy);
                              };
                          },
                          [](const command_arguments::resize& resize) -> Command {
                              return [resize](Server* server) {
                                  return commands::resize(server, resize.width, resize.height);
                              };
                          },
                          [](const command_arguments::config_mouse_mod& mouse_mod) -> Command {
                              uint32_t modifiers = modifier_array_to_mask(mouse_mod.modifiers);
                              return [modifiers](Server* server) {
                                  return commands::config_mouse_mod(server, modifiers);
                              };
                          },
                          [](const command_arguments::cycle_width&) -> Command {
                              return commands::cycle_width;
                          },
                      },
                      command_data);
}
