/*
1. json文件的格式：
The general syntax of the JSON output created by this command is as follows:

    {
      "creator": "Yosys <version info>",
      "modules": {
        <module_name>: {
          "attributes": {
            <attribute_name>: <attribute_value>,
            ...
          },
          "parameter_default_values": {
            <parameter_name>: <parameter_value>,
            ...
          },
          "ports": {
            <port_name>: <port_details>,
            ...
          },
          "cells": {
            <cell_name>: <cell_details>,
            ...
          },
          "memories": {
            <memory_name>: <memory_details>,
            ...
          },
          "netnames": {
            <net_name>: <net_details>,
            ...
          }
        }
      },
      "models": {
        ...
      },
    }

Where <port_details> is:

    {
      "direction": <"input" | "output" | "inout">,
      "bits": <bit_vector>
      "offset": <the lowest bit index in use, if non-0>
      "upto": <1 if the port bit indexing is MSB-first>
      "signed": <1 if the port is signed>
    }

The "offset" and "upto" fields are skipped if their value would be 0.
They don't affect connection semantics, and are only used to preserve original
HDL bit indexing.
And <cell_details> is:

    {
      "hide_name": <1 | 0>,
      "type": <cell_type>,
      "model": <AIG model name, if -aig option used>,
      "parameters": {
        <parameter_name>: <parameter_value>,
        ...
      },
      "attributes": {
        <attribute_name>: <attribute_value>,
        ...
      },
      "port_directions": {
        <port_name>: <"input" | "output" | "inout">,
        ...
      },
      "connections": {
        <port_name>: <bit_vector>,
        ...
      },
    }

And <memory_details> is:

    {
      "hide_name": <1 | 0>,
      "attributes": {
        <attribute_name>: <attribute_value>,
        ...
      },
      "width": <memory width>
      "start_offset": <the lowest valid memory address>
      "size": <memory size>
    }

And <net_details> is:

    {
      "hide_name": <1 | 0>,
      "bits": <bit_vector>
      "offset": <the lowest bit index in use, if non-0>
      "upto": <1 if the port bit indexing is MSB-first>
      "signed": <1 if the port is signed>
    }

The "hide_name" fields are set to 1 when the name of this cell or net is
automatically created and is likely not of interest for a regular user.

The "port_directions" section is only included for cells for which the
interface is known.

Module and cell ports and nets can be single bit wide or vectors of multiple
bits. Each individual signal bit is assigned a unique integer. The <bit_vector>
values referenced above are vectors of this integers. Signal bits that are
connected to a constant driver are denoted as string "0", "1", "x", or
"z" instead of a number.

Bit vectors (including integers) are written as string holding the binary
representation of the value. Strings are written as strings, with an appended
blank in cases of strings of the form /[01xz]* .

2. json里面需要用到的信息：
1. modules 名称
2. module 自身所有的 port ，port 的名称、方向、bits
3. module 里面所有的 cell ，cell 的名称、hide、type、port directions、connections
*/

#ifndef CONFIG_HH
#define CONFIG_HH

#include <cstddef>
#include <stdexcept>
#include <string>
#include <map>
#include <vector>
#include <variant>


namespace parser {

enum class PortDirection {
    INPUT,
    OUTPUT,
    INOUT
};

static std::string port_dir_to_string(PortDirection direction) {
  switch (direction) {
    case PortDirection::INPUT: return "input";
    case PortDirection::INOUT: return "inout";
    case PortDirection::OUTPUT: return "output";
    default: throw std::runtime_error("unknown port direction!");
  }
}


struct Port {
    std::string _name;
    PortDirection _direction;
    std::vector<std::variant<int, std::string>> _bits;

    std::string to_string() {
      std::string msg{"{port name = " + _name + "\n"};
      msg += "direction: " + port_dir_to_string(_direction) + "\n";
      msg += "bits: ";
      for (auto& bit: _bits) {
        msg += std::holds_alternative<std::string>(bit) ? std::get<std::string>(bit) : std::to_string(std::get<int>(bit));
        msg += " ";
      }
      msg += "\n}\n";

      return msg;
    }
};


struct Cell {
    std::string _name;
    bool _hide;
    std::string _type;
    std::map<std::string, PortDirection> _port_directions;
    std::map<std::string, std::vector<std::variant<int, std::string>>> _connections;

    std::string to_string() {
      std::string msg{"{cell name = " + _name + "\n"};
      msg += "hide_name: " + (_hide ? std::to_string(1) : std::to_string(0)) + "\n";
      msg += "type: " + _type + "\n";
      msg += "port direction: \n";
      for (auto& [name, dir]: _port_directions) {
        msg += "    " + name + ": " + port_dir_to_string(dir) + "\n";
      }
      msg += "connections: \n";
      for (auto& [name, bits]: _connections) {
        for (auto bit: bits) {
          msg += "    " + name + ": " + (std::holds_alternative<std::string>(bit) ? std::get<std::string>(bit) : std::to_string(std::get<int>(bit)));
          msg += "\n";
        }
      }
      msg += "}\n";

      return msg;
    }
};


struct Module {
    std::string _name;
    std::map<std::string, Port> _ports;
    std::map<std::string, Cell> _cells;

    std::string to_string() {
      std::string msg{"module name = " + _name + "\n"};
      for (auto& [name, port]: _ports) {
        msg += port.to_string();
      }
      for (auto& [name, cell]: _cells) {
        msg += cell.to_string();
      }
      msg += "\n";

      return msg;
    }
};


}

#endif  // CONFIG_HH

