# A bunch of this code is taken from WineVulkan
# (https://source.winehq.org/git/wine.git/tree/HEAD:/dlls/winevulkan),
# which is a part of the Wine project (https://www.winehq.org/)
# which is licensed under LGPL v2.1; the licence is as follows:
#
# From https://source.winehq.org/git/wine.git/blob/HEAD:/LICENSE :-
#
#   Copyright (c) 1993-2022 the Wine project authors (see the file AUTHORS
#   for a complete list)
#   
#   Wine is free software; you can redistribute it and/or modify it under
#   the terms of the GNU Lesser General Public License as published by the
#   Free Software Foundation; either version 2.1 of the License, or (at
#   your option) any later version. 
#   
#   This program is distributed in the hope that it will be useful, but
#   WITHOUT ANY WARRANTY; without even the implied warranty of
#   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
#   Lesser General Public License for more details.
#   
#   A copy of the GNU Lesser General Public License is included in the
#   Wine distribution in the file COPYING.LIB. If you did not receive this
#   copy, write to the Free Software Foundation, Inc., 51 Franklin St,
#   Fifth Floor, Boston, MA 02110-1301, USA.
#

from collections import OrderedDict
from collections.abc import Sequence
from enum import Enum
import re
import xml.etree.ElementTree as ET
import logging

# Extension enum values start at a certain offset (EXT_BASE).
# Relative to the offset each extension has a block (EXT_BLOCK_SIZE)
# of values.
# Start for a given extension is:
# EXT_BASE + (extension_number-1) * EXT_BLOCK_SIZE
EXT_BASE = 1000000000
EXT_BLOCK_SIZE = 1000

VK_VERSION = (1, 3)

CORE_EXTENSIONS = [
    "VK_KHR_display",
    "VK_KHR_display_swapchain",
    "VK_KHR_get_surface_capabilities2",
    "VK_KHR_surface",
    "VK_KHR_swapchain",

    "VK_KHR_xlib_surface",
    "VK_KHR_xcb_surface",
    "VK_KHR_wayland_surface",
    "VK_KHR_win32_surface",
    "VK_EXT_headless_surface",
]

LOGGER = logging.Logger("vkroots")
LOGGER.addHandler(logging.StreamHandler())

class Direction(Enum):
    """ Parameter direction: input, output, input_output. """
    INPUT = 1
    OUTPUT = 2
    INPUT_OUTPUT = 3

class VkBaseType(object):
    def __init__(self, name, _type, alias=None, requires=None):
        """ Vulkan base type class.

        VkBaseType is mostly used by Vulkan to define its own
        base types like VkFlags through typedef out of e.g. uint32_t.

        Args:
            name (:obj:'str'): Name of the base type.
            _type (:obj:'str'): Underlying type
            alias (bool): type is an alias or not.
            requires (:obj:'str', optional): Other types required.
                Often bitmask values pull in a *FlagBits type.
        """
        self.name = name
        self.type = _type
        self.alias = alias
        self.requires = requires
        self.required = False

    def definition(self):
        # Definition is similar for alias or non-alias as type
        # is already set to alias.
        if not self.type is None:
            return "typedef {0} {1};\n".format(self.type, self.name)
        else:
            return "struct {0};\n".format(self.name)

    def is_alias(self):
        return bool(self.alias)


class VkConstant(object):
    def __init__(self, name, value):
        self.name = name
        self.value = value

    def definition(self):
        text = "#define {0} {1}\n".format(self.name, self.value)
        return text


class VkDefine(object):
    def __init__(self, name, value):
        self.name = name
        self.value = value

    @staticmethod
    def from_xml(define):
        name_elem = define.find("name")

        if name_elem is None:
            # <type category="define" name="some_name">some_value</type>
            # At the time of writing there is only 1 define of this category
            # 'VK_DEFINE_NON_DISPATCHABLE_HANDLE'.
            name = define.attrib.get("name")

            # We override behavior of VK_DEFINE_NON_DISPATCHABLE handle as the default
            # definition various between 64-bit (uses pointers) and 32-bit (uses uint64_t).
            # This complicates TRACEs in the thunks, so just use uint64_t.
            if name == "VK_DEFINE_NON_DISPATCHABLE_HANDLE":
                value = "#define VK_DEFINE_NON_DISPATCHABLE_HANDLE(object) typedef uint64_t object;"
            else:
                value = define.text
            return VkDefine(name, value)

        # With a name element the structure is like:
        # <type category="define"><name>some_name</name>some_value</type>
        name = name_elem.text

        # Perform minimal parsing for Vulkan constants, which we don't need, but are referenced
        # elsewhere in vk.xml.
        # - VK_API_VERSION is a messy, deprecated constant and we don't want generate code for it.
        # - AHardwareBuffer/ANativeWindow are forward declarations for Android types, which leaked
        #   into the define region.
        if name in ["VK_API_VERSION", "AHardwareBuffer", "ANativeWindow", "CAMetalLayer"]:
            return VkDefine(name, None)

        # The body of the define is basically unstructured C code. It is not meant for easy parsing.
        # Some lines contain deprecated values or comments, which we try to filter out.
        value = ""
        for line in define.text.splitlines():
            # Skip comments or deprecated values.
            if "//" in line:
                continue
            value += line

        for child in define:
            value += child.text
            if child.tail is not None:
                # Split comments for VK_API_VERSION_1_0 / VK_API_VERSION_1_1
                if "//" in child.tail:
                    value += child.tail.split("//")[0]
                else:
                    value += child.tail

        return VkDefine(name, value.rstrip(' '))

    def definition(self):
        if self.value is None:
            return ""

        # Nothing to do as the value was already put in the right form during parsing.
        return "{0}\n".format(self.value)

class VkEnum(object):
    def __init__(self, name, bitwidth, alias=None):
        if not bitwidth in [32, 64]:
            LOGGER.error("unknown bitwidth {0} for {1}".format(bitwidth, name))
        self.extensions = []
        self.name = name
        self.bitwidth = bitwidth
        self.values = [] if alias == None else alias.values
        self.required = False
        self.alias = alias
        self.aliased_by = []

    @staticmethod
    def from_alias(enum, alias):
        name = enum.attrib.get("name")
        aliasee = VkEnum(name, alias.bitwidth, alias=alias)

        alias.add_aliased_by(aliasee)
        return aliasee

    @staticmethod
    def from_xml(enum):
        name = enum.attrib.get("name")
        bitwidth = int(enum.attrib.get("bitwidth", "32"))
        result = VkEnum(name, bitwidth)

        for v in enum.findall("enum"):
            value_name = v.attrib.get("name")
            # Value is either a value or a bitpos, only one can exist.
            value = v.attrib.get("value")
            alias_name = v.attrib.get("alias")
            if alias_name:
                result.create_alias(value_name, alias_name)
            elif value:
                result.create_value(value_name, value)
            else:
                # bitmask
                result.create_bitpos(value_name, int(v.attrib.get("bitpos")))

        if bitwidth == 32:
            # vulkan.h contains a *_MAX_ENUM value set to 32-bit at the time of writing,
            # which is to prepare for extensions as they can add values and hence affect
            # the size definition.
            max_name = re.sub(r'([0-9a-z_])([A-Z0-9])',r'\1_\2', name).upper() + "_MAX_ENUM"
            result.create_value(max_name, "0x7fffffff")

        return result

    def create_alias(self, name, alias_name):
        """ Create an aliased value for this enum """
        self.add(VkEnumValue(name, self.bitwidth, alias=alias_name))

    def create_value(self, name, value):
        """ Create a new value for this enum """
        # Some values are in hex form. We want to preserve the hex representation
        # at least when we convert back to a string. Internally we want to use int.
        hex = "0x" in value
        self.add(VkEnumValue(name, self.bitwidth, value=int(value, 0), hex=hex))

    def fixup_64bit_aliases(self):
        """ Replace 64bit aliases with literal values """
        # Older GCC versions need a literal to initialize a static const uint64_t
        # which is what we use for 64bit bitmasks.
        if self.bitwidth != 64:
            return
        for value in self.values:
            if not value.is_alias():
                continue
            alias = next(x for x in self.values if x.name == value.alias)
            value.hex = alias.hex
            value.value = alias.value

    def create_bitpos(self, name, pos):
        """ Create a new bitmask value for this enum """
        self.add(VkEnumValue(name, self.bitwidth, value=(1 << pos), hex=True))

    def add(self, value):
        """ Add a value to enum. """

        # Extensions can add new enum values. When an extension is promoted to Core
        # the registry defines the value twice once for old extension and once for
        # new Core features. Add the duplicate if it's explicitly marked as an
        # alias, otherwise ignore it.
        for v in self.values:
            if not value.is_alias() and v.value == value.value:
                LOGGER.debug("Adding duplicate enum value {0} to {1}".format(v, self.name))
                return
        # Avoid adding duplicate aliases multiple times
        if not any(x.name == value.name for x in self.values):
            self.values.append(value)

    def definition(self):
        if self.is_alias():
            return ""

        default_value = 0x7ffffffe if self.bitwidth == 32 else 0xfffffffffffffffe

        # Print values sorted, values can have been added in a random order.
        values = sorted(self.values, key=lambda value: value.value if value.value is not None else default_value)

        if self.bitwidth == 32:
            text = "typedef enum {0}\n{{\n".format(self.name)
            for value in values:
                text += "    {0},\n".format(value.definition())
            text += "}} {0};\n".format(self.name)
        elif self.bitwidth == 64:
            text = "typedef VkFlags64 {0};\n\n".format(self.name)
            for value in values:
                text += "static const {0} {1};\n".format(self.name, value.definition())

        for aliasee in self.aliased_by:
            text += "typedef {0} {1};\n".format(self.name, aliasee.name)

        text += "\n"
        return text

    def is_alias(self):
        return bool(self.alias)

    def add_aliased_by(self, aliasee):
        self.aliased_by.append(aliasee)


class VkEnumValue(object):
    def __init__(self, name, bitwidth, value=None, hex=False, alias=None):
        self.name = name
        self.bitwidth = bitwidth
        self.value = value
        self.hex = hex
        self.alias = alias

    def __repr__(self):
        postfix = "ull" if self.bitwidth == 64 else ""
        if self.is_alias() and self.value == None:
            return "{0}={1}".format(self.name, self.alias)
        return "{0}={1}{2}".format(self.name, self.value, postfix)

    def definition(self):
        """ Convert to text definition e.g. VK_FOO = 1 """
        postfix = "ull" if self.bitwidth == 64 else ""
        if self.is_alias() and self.value == None:
            return "{0} = {1}".format(self.name, self.alias)

        # Hex is commonly used for FlagBits and sometimes within
        # a non-FlagBits enum for a bitmask value as well.
        if self.hex:
            return "{0} = 0x{1:08x}{2}".format(self.name, self.value, postfix)
        else:
            return "{0} = {1}{2}".format(self.name, self.value, postfix)

    def is_alias(self):
        return self.alias is not None


class VkFunction(object):
    def __init__(self, _type=None, name=None, params=[], extensions=[], alias=None):
        self.extensions = []
        self.name = name
        self.type = _type
        self.params = params
        self.alias = alias

    @staticmethod
    def from_alias(command, alias):
        """ Create VkFunction from an alias command.

        Args:
            command: xml data for command
            alias (VkFunction): function to use as a base for types / parameters.

        Returns:
            VkFunction
        """
        func_name = command.attrib.get("name")
        func_type = alias.type
        params = alias.params

        return VkFunction(_type=func_type, name=func_name, params=params, alias=alias)

    @staticmethod
    def from_xml(command, types):
        proto = command.find("proto")
        func_name = proto.find("name").text
        func_type = proto.find("type").text

        params = []
        for param in command.findall("param"):
            vk_param = VkParam.from_xml(param, types)
            params.append(vk_param)

        return VkFunction(_type=func_type, name=func_name, params=params)


    def is_alias(self):
        return bool(self.alias)

    def is_core_func(self):
        """ Returns whether the function is a Vulkan core function.
        Core functions are APIs defined by the Vulkan spec to be part of the
        Core API as well as several KHR WSI extensions.
        """

        if not self.extensions:
            return True

        return any(ext in self.extensions for ext in CORE_EXTENSIONS)

    def is_device_func(self):
        # If none of the other, it must be a device function
        return not self.is_global_func() and not self.is_instance_func() and not self.is_phys_dev_func()

    def is_global_func(self):
        # Global functions are not passed a dispatchable object.
        if self.params[0].is_dispatchable():
            return False
        return True

    def is_instance_func(self):
        # Instance functions are passed VkInstance.
        if self.name == "vkCreateInstance":
            return True
        if self.params[0].type == "VkPhysicalDevice" and self.is_core_func():
            return True
        if self.params[0].type == "VkInstance":
            return True
        return False

    def is_phys_dev_func(self):
        # Physical device functions are passed VkPhysicalDevice.
        # BUT they aren't phys dev functions if they are in core.
        # *sigh*
        if not self.is_core_func() and self.params[0].type == "VkPhysicalDevice":
            return True
        return False

    def get_func_type(self):
        if self.is_instance_func():
            return "Instance"
        if self.is_phys_dev_func():
            return "PhysicalDevice"
        if self.is_device_func():
            return "Device"

    def destroyed_dispatchable(self):
        # Physical device functions are passed VkPhysicalDevice.
        if self.name == "vkDestroyedInstance":
            return "*pInstance"
        if self.name == "vkDestroyPhysicalDevice":
            return "*pPhysicalDevice"
        if self.name == "vkDestroyDevice":
            return "*pDevice"
        return False

    def prototype(self, call_conv=None, prefix=None, postfix=None):
        """ Generate prototype for given function.

        Args:
            call_conv (str, optional): calling convention e.g. WINAPI
            prefix (str, optional): prefix to append prior to function name e.g. vkFoo -> wine_vkFoo
            postfix (str, optional): text to append after function name but prior to semicolon e.g. DECLSPEC_HIDDEN
        """

        proto = "{0}".format(self.type)

        if call_conv is not None:
            proto += " {0}".format(call_conv)

        if prefix is not None:
            proto += " {0}{1}(".format(prefix, self.name)
        else:
            proto += " {0}(".format(self.name)

        # Add all the parameters.
        proto += ", ".join([p.definition() for p in self.params])

        if postfix is not None:
            proto += ") {0}".format(postfix)
        else:
            proto += ")"

        return proto

class VkFunctionPointer(object):
    def __init__(self, _type, name, members, forward_decls):
        self.name = name
        self.members = members
        self.type = _type
        self.required = False
        self.forward_decls = forward_decls

    @staticmethod
    def from_xml(funcpointer):
        members = []
        begin = None

        for t in funcpointer.findall("type"):
            # General form:
            # <type>void</type>*       pUserData,
            # Parsing of the tail (anything past </type>) is tricky since there
            # can be other data on the next line like: const <type>int</type>..

            const = True if begin and "const" in begin else False
            _type = t.text
            lines = t.tail.split(",\n")
            if lines[0][0] == "*":
                pointer = "*"
                name = lines[0][1:].strip()
            else:
                pointer = None
                name = lines[0].strip()

            # Filter out ); if it is contained.
            name = name.partition(");")[0]

            # If tail encompasses multiple lines, assign the second line to begin
            # for the next line.
            try:
                begin = lines[1].strip()
            except IndexError:
                begin = None

            members.append(VkMember(const=const, _type=_type, pointer=pointer, name=name))

        _type = funcpointer.text
        name = funcpointer.find("name").text
        if "requires" in funcpointer.attrib:
            forward_decls = funcpointer.attrib.get("requires").split(",")
        else:
            forward_decls = []
        return VkFunctionPointer(_type, name, members, forward_decls)

    def definition(self):
        text = ""
        # forward declare required structs
        for decl in self.forward_decls:
            text += "typedef struct {0} {0};\n".format(decl)

        text += "{0} {1})(\n".format(self.type, self.name)

        first = True
        if len(self.members) > 0:
            for m in self.members:
                if first:
                    text += "    " + m.definition()
                    first = False
                else:
                    text += ",\n    " + m.definition()
        else:
            # Just make the compiler happy by adding a void parameter.
            text += "void"
        text += ");\n"
        return text

    def is_alias(self):
        return False

class VkHandle(object):
    def __init__(self, name, _type, parent, alias=None):
        self.name = name
        self.type = _type
        self.parent = parent
        self.alias = alias
        self.required = False
        self.object_type = None

    @staticmethod
    def from_alias(handle, alias):
        name = handle.attrib.get("name")
        return VkHandle(name, alias.type, alias.parent, alias=alias)

    @staticmethod
    def from_xml(handle):
        name = handle.find("name").text
        _type = handle.find("type").text
        parent = handle.attrib.get("parent") # Most objects have a parent e.g. VkQueue has VkDevice.
        return VkHandle(name, _type, parent)

    def dispatch_table(self):
        if not self.is_dispatchable():
            return None

        if self.parent is None:
            # Should only happen for VkInstance
            return "funcs"
        elif self.name == "VkDevice":
            # VkDevice has VkInstance as a parent, but has its own dispatch table.
            return "funcs"
        elif self.parent in ["VkInstance", "VkPhysicalDevice"]:
            return "instance->funcs"
        elif self.parent in ["VkDevice", "VkCommandPool"]:
            return "device->funcs"
        else:
            LOGGER.error("Unhandled dispatchable parent: {0}".format(self.parent))

    def definition(self):
        """ Generates handle definition e.g. VK_DEFINE_HANDLE(vkInstance) """

        # Legacy types are typedef'ed to the new type if they are aliases.
        if self.is_alias():
            return "typedef {0} {1};\n".format(self.alias.name, self.name)

        return "{0}({1})\n".format(self.type, self.name)

    def is_alias(self):
        return self.alias is not None

    def is_dispatchable(self):
        """ Some handles like VkInstance, VkDevice are dispatchable objects,
        which means they contain a dispatch table of function pointers.
        """
        return self.type == "VK_DEFINE_HANDLE"

    def is_required(self):
        return self.required

    def native_handle(self, name):
        """ Provide access to the native handle of a wrapped object. """

        if self.name == "VkCommandPool":
            return "wine_cmd_pool_from_handle({0})->command_pool".format(name)
        if self.name == "VkDebugUtilsMessengerEXT":
            return "wine_debug_utils_messenger_from_handle({0})->debug_messenger".format(name)
        if self.name == "VkDebugReportCallbackEXT":
            return "wine_debug_report_callback_from_handle({0})->debug_callback".format(name)
        if self.name == "VkSurfaceKHR":
            return "wine_surface_from_handle({0})->surface".format(name)

        native_handle_name = None

        if self.name == "VkCommandBuffer":
            native_handle_name = "command_buffer"
        if self.name == "VkDevice":
            native_handle_name = "device"
        if self.name == "VkInstance":
            native_handle_name = "instance"
        if self.name == "VkPhysicalDevice":
            native_handle_name = "phys_dev"
        if self.name == "VkQueue":
            native_handle_name = "queue"

        if native_handle_name:
            return "{0}->{1}".format(name, native_handle_name)

        if self.is_dispatchable():
            LOGGER.error("Unhandled native handle for: {0}".format(self.name))
        return None

    def driver_handle(self, name):
        """ Provide access to the handle that should be passed to the wine driver """

        if self.name == "VkSurfaceKHR":
            return "wine_surface_from_handle({0})->driver_surface".format(name)

        return self.native_handle(name)

    def is_wrapped(self):
        return self.native_handle("test") is not None

class VkMember(object):
    def __init__(self, const=False, struct_fwd_decl=False,_type=None, pointer=None, name=None, array_len=None,
            dyn_array_len=None, optional=False, values=None):
        self.const = const
        self.struct_fwd_decl = struct_fwd_decl
        self.name = name
        self.pointer = pointer
        self.type = _type
        self.type_info = None
        self.array_len = array_len
        self.dyn_array_len = dyn_array_len
        self.optional = optional
        self.values = values

    def __eq__(self, other):
        """ Compare member based on name against a string.

        This method is for convenience by VkStruct, which holds a number of members and needs quick checking
        if certain members exist.
        """

        return self.name == other

    def __repr__(self):
        return "{0} {1} {2} {3} {4} {5} {6}".format(self.const, self.struct_fwd_decl, self.type, self.pointer,
                self.name, self.array_len, self.dyn_array_len)

    @staticmethod
    def from_xml(member):
        """ Helper function for parsing a member tag within a struct or union. """

        name_elem = member.find("name")
        type_elem = member.find("type")

        const = False
        struct_fwd_decl = False
        member_type = None
        pointer = None
        array_len = None

        values = member.get("values")

        if member.text:
            if "const" in member.text:
                const = True

            # Some members contain forward declarations:
            # - VkBaseInstructure has a member "const struct VkBaseInStructure *pNext"
            # - VkWaylandSurfaceCreateInfoKHR has a member "struct wl_display *display"
            if "struct" in member.text:
                struct_fwd_decl = True

        if type_elem is not None:
            member_type = type_elem.text
            if type_elem.tail is not None:
                pointer = type_elem.tail.strip() if type_elem.tail.strip() != "" else None

        # Name of other member within, which stores the number of
        # elements pointed to be by this member.
        dyn_array_len = member.get("len")

        # Some members are optional, which is important for conversion code e.g. not dereference NULL pointer.
        optional = True if member.get("optional") else False

        # Usually we need to allocate memory for dynamic arrays. We need to do the same in a few other cases
        # like for VkCommandBufferBeginInfo.pInheritanceInfo. Just threat such cases as dynamic arrays of
        # size 1 to simplify code generation.
        if dyn_array_len is None and pointer is not None:
            dyn_array_len = 1

        # Some members are arrays, attempt to parse these. Formats include:
        # <member><type>char</type><name>extensionName</name>[<enum>VK_MAX_EXTENSION_NAME_SIZE</enum>]</member>
        # <member><type>uint32_t</type><name>foo</name>[4]</member>
        if name_elem.tail and name_elem.tail[0] == '[':
            LOGGER.debug("Found array type")
            enum_elem = member.find("enum")
            if enum_elem is not None:
                array_len = enum_elem.text
            else:
                # Remove brackets around length
                array_len = name_elem.tail.strip("[]")

        return VkMember(const=const, struct_fwd_decl=struct_fwd_decl, _type=member_type, pointer=pointer, name=name_elem.text,
                array_len=array_len, dyn_array_len=dyn_array_len, optional=optional, values=values)

    def copy(self, input, output, direction):
        """ Helper method for use by conversion logic to generate a C-code statement to copy this member. """

        if self.is_static_array():
            bytes_count = "{0} * sizeof({1})".format(self.array_len, self.type)
            return "memcpy({0}{1}, {2}{1}, {3});\n".format(output, self.name, input, bytes_count)
        else:
            return "{0}{1} = {2}{1};\n".format(output, self.name, input)

    def definition(self, align=False, conv=False):
        """ Generate prototype for given function.

        Args:
            align (bool, optional): Enable alignment if a type needs it. This adds WINE_VK_ALIGN(8) to a member.
            conv (bool, optional): Enable conversion if a type needs it. This appends '_host' to the name.
        """

        text = ""
        if self.is_const():
            text += "const "

        if self.is_struct_forward_declaration():
            text += "struct "

        if conv and self.is_struct():
            text += "{0}_host".format(self.type)
        else:
            text += self.type

        if self.is_pointer():
            text += " {0}{1}".format(self.pointer, self.name)
        else:
            text += " " + self.name

        if self.is_static_array():
            text += "[{0}]".format(self.array_len)

        return text

    def is_const(self):
        return self.const

    def is_dynamic_array(self):
        """ Returns if the member is an array element.
        Vulkan uses this for dynamically sized arrays for which
        there is a 'count' parameter.
        """
        return self.dyn_array_len is not None

    def is_handle(self):
        return self.type_info["category"] == "handle"

    def is_pointer(self):
        return self.pointer is not None

    def is_static_array(self):
        """ Returns if the member is an array.
        Vulkan uses this often for fixed size arrays in which the
        length is part of the member.
        """
        return self.array_len is not None

    def is_struct(self):
        return self.type_info["category"] == "struct"

    def is_struct_forward_declaration(self):
        return self.struct_fwd_decl

    def is_union(self):
        return self.type_info["category"] == "union"

    def set_type_info(self, type_info):
        """ Helper function to set type information from the type registry.
        This is needed, because not all type data is available at time of
        parsing.
        """
        self.type_info = type_info


class VkParam(object):
    """ Helper class which describes a parameter to a function call. """

    def __init__(self, type_info, const=None, pointer=None, name=None, array_len=None, dyn_array_len=None):
        self.const = const
        self.name = name
        self.array_len = array_len
        self.dyn_array_len = dyn_array_len
        self.pointer = pointer
        self.type_info = type_info
        self.type = type_info["name"] # For convenience
        self.handle = type_info["data"] if type_info["category"] == "handle" else None
        self.struct = type_info["data"] if type_info["category"] == "struct" else None

        self._set_direction()
        self._set_format_string()

    def __repr__(self):
        return "{0} {1} {2} {3} {4} {5}".format(self.const, self.type, self.pointer, self.name, self.array_len, self.dyn_array_len)

    @staticmethod
    def from_xml(param, types):
        """ Helper function to create VkParam from xml. """

        # Parameter parsing is slightly tricky. All the data is contained within
        # a param tag, but some data is within subtags while others are text
        # before or after the type tag.
        # Common structure:
        # <param>const <type>char</type>* <name>pLayerName</name></param>

        name_elem = param.find("name")
        array_len = None
        name = name_elem.text
        # Tail contains array length e.g. for blendConstants param of vkSetBlendConstants
        if name_elem.tail is not None:
            array_len = name_elem.tail.strip("[]")

        # Name of other parameter in function prototype, which stores the number of
        # elements pointed to be by this parameter.
        dyn_array_len = param.get("len", None)

        const = param.text.strip() if param.text else None
        type_elem = param.find("type")
        pointer = type_elem.tail.strip() if type_elem.tail.strip() != "" else None

        # Since we have parsed all types before hand, this should not happen.
        type_info = types.get(type_elem.text, None)
        if type_info is None:
            LOGGER.err("type info not found for: {0}".format(type_elem.text))

        return VkParam(type_info, const=const, pointer=pointer, name=name, array_len=array_len, dyn_array_len=dyn_array_len)

    def _set_direction(self):
        """ Internal helper function to set parameter direction (input/output/input_output). """

        # The parameter direction needs to be determined from hints in vk.xml like returnedonly,
        # parameter constness and other heuristics.
        # For now we need to get this right for structures as we need to convert these, we may have
        # missed a few other edge cases (e.g. count variables).
        # See also https://github.com/KhronosGroup/Vulkan-Docs/issues/610

        if not self.is_pointer():
            self._direction = Direction.INPUT
        elif self.is_const() and self.is_pointer():
            self._direction = Direction.INPUT
        elif self.is_struct():
            if not self.struct.returnedonly:
                self._direction = Direction.INPUT
                return

            # Returnedonly hints towards output, however in some cases
            # it is inputoutput. In particular if pNext / sType exist,
            # which are used to link in other structures without having
            # to introduce new APIs. E.g. vkGetPhysicalDeviceProperties2KHR.
            if "pNext" in self.struct:
                self._direction = Direction.INPUT_OUTPUT
                return

            self._direction = Direction.OUTPUT
        else:
            # This should mostly be right. Count variables can be inout, but we don't care about these yet.
            self._direction = Direction.OUTPUT

    def _set_format_string(self):
        """ Internal helper function to be used by constructor to set format string. """

        # Determine a format string used by code generation for traces.
        # 64-bit types need a conversion function.
        self.format_conv = None
        if self.is_static_array() or self.is_pointer():
            self.format_str = "%p"
        else:
            if self.type_info["category"] in ["bitmask"]:
                # Since 1.2.170 bitmasks can be 32 or 64-bit, check the basetype.
                if self.type_info["data"].type == "VkFlags64":
                    self.format_str = "0x%s"
                    self.format_conv = "wine_dbgstr_longlong({0})"
                else:
                    self.format_str = "%#x"
            elif self.type_info["category"] in ["enum"]:
                self.format_str = "%#x"
            elif self.is_handle():
                # We use uint64_t for non-dispatchable handles as opposed to pointers
                # for dispatchable handles.
                if self.handle.is_dispatchable():
                    self.format_str = "%p"
                else:
                    self.format_str = "0x%s"
                    self.format_conv = "wine_dbgstr_longlong({0})"
            elif self.type == "float":
                self.format_str = "%f"
            elif self.type == "int":
                self.format_str = "%d"
            elif self.type == "int32_t":
                self.format_str = "%d"
            elif self.type == "size_t":
                self.format_str = "0x%s"
                self.format_conv = "wine_dbgstr_longlong({0})"
            elif self.type in ["uint16_t", "uint32_t", "VkBool32"]:
                self.format_str = "%u"
            elif self.type in ["uint64_t", "VkDeviceAddress", "VkDeviceSize"]:
                self.format_str = "0x%s"
                self.format_conv = "wine_dbgstr_longlong({0})"
            elif self.type == "HANDLE":
                self.format_str = "%p"
            elif self.type in ["VisualID", "xcb_visualid_t", "RROutput"]:
                # Don't care about Linux specific types.
                self.format_str = ""
            else:
                LOGGER.warn("Unhandled type: {0}".format(self.type_info))

    def copy(self, direction):
        if direction == Direction.INPUT:
            if self.is_dynamic_array():
                return "    {0}_host = convert_{1}_array_win_to_host({0}, {2});\n".format(self.name, self.type, self.dyn_array_len)
            else:
                return "    convert_{0}_win_to_host({1}, &{1}_host);\n".format(self.type, self.name)
        else:
            if self.is_dynamic_array():
                LOGGER.error("Unimplemented output conversion for: {0}".format(self.name))
            else:
                return "    convert_{0}_host_to_win(&{1}_host, {1});\n".format(self.type, self.name)

    def definition(self, postfix=None):
        """ Return prototype for the parameter. E.g. 'const char *foo' """

        proto = ""
        if self.const:
            proto += self.const + " "

        proto += self.type

        if self.is_pointer():
            proto += " {0}{1}".format(self.pointer, self.name)
        else:
            proto += " " + self.name

        # Allows appending something to the variable name useful for
        # win32 to host conversion.
        if postfix is not None:
            proto += postfix

        if self.is_static_array():
            proto += "[{0}]".format(self.array_len)

        return proto

    def direction(self):
        """ Returns parameter direction: input, output, input_output.

        Parameter direction in Vulkan is not straight-forward, which this function determines.
        """

        return self._direction

    def dispatch_table(self):
        """ Return functions dispatch table pointer for dispatchable objects. """

        if not self.is_dispatchable():
            return None

        return "{0}->{1}".format(self.name, self.handle.dispatch_table())

    def format_string(self):
        return self.format_str

    def is_const(self):
        return self.const is not None

    def is_dynamic_array(self):
        return self.dyn_array_len is not None

    def is_dispatchable(self):
        if not self.is_handle():
            return False

        return self.handle.is_dispatchable()

    def is_handle(self):
        return self.handle is not None

    def is_pointer(self):
        return self.pointer is not None

    def is_static_array(self):
        return self.array_len is not None

    def is_struct(self):
        return self.struct is not None

    def spec(self):
        """ Generate spec file entry for this parameter. """

        if self.is_pointer() and self.type == "char":
            return "str"
        if self.is_dispatchable() or self.is_pointer() or self.is_static_array():
            return "ptr"
        if self.type_info["category"] in ["bitmask"]:
            # Since 1.2.170 bitmasks can be 32 or 64-bit, check the basetype.
            if self.type_info["data"].type == "VkFlags64":
                return "int64"
            else:
                return "long"
        if self.type_info["category"] in ["enum"]:
            return "long"
        if self.is_handle() and not self.is_dispatchable():
            return "int64"
        if self.type == "float":
            return "float"
        if self.type in ["int", "int32_t", "size_t", "uint16_t", "uint32_t", "VkBool32"]:
            return "long"
        if self.type in ["uint64_t", "VkDeviceSize"]:
            return "int64"

        LOGGER.error("Unhandled spec conversion for type: {0}".format(self.type))

    def variable(self, conv=False):
        """ Returns 'glue' code during generation of a function call on how to access the variable.
        This function handles various scenarios such as 'unwrapping' if dispatchable objects and
        renaming of parameters in case of win32 -> host conversion.

        Args:
            conv (bool, optional): Enable conversion if the param needs it. This appends '_host' to the name.
        """

        # Hack until we enable allocation callbacks from ICD to application. These are a joy
        # to enable one day, because of calling convention conversion.
        if "VkAllocationCallbacks" in self.type:
            LOGGER.debug("TODO: setting NULL VkAllocationCallbacks for {0}".format(self.name))
            return "NULL"

        # We need to pass the native handle to the native Vulkan calls and
        # the wine driver's handle to calls which are wrapped by the driver.
        driver_handle = self.handle.driver_handle(self.name) if self.is_handle() else None
        return driver_handle if driver_handle else self.name


class VkStruct(Sequence):
    """ Class which represents the type union and struct. """

    def __init__(self, name, members, returnedonly, structextends, alias=None, union=False):
        self.name = name
        self.members = members
        self.returnedonly = returnedonly
        self.structextends = structextends
        self.required = False
        self.alias = alias
        self.union = union
        self.type_info = None # To be set later.
        self.struct_extensions = []
        self.aliased_by = []

    def __getitem__(self, i):
        return self.members[i]

    def __len__(self):
        return len(self.members)

    @staticmethod
    def from_alias(struct, alias):
        name = struct.attrib.get("name")
        aliasee = VkStruct(name, alias.members, alias.returnedonly, alias.structextends, alias=alias)

        alias.add_aliased_by(aliasee)
        return aliasee

    @staticmethod
    def from_xml(struct):
        # Unions and structs are the same parsing wise, but we need to
        # know which one we are dealing with later on for code generation.
        union = True if struct.attrib["category"] == "union" else False

        name = struct.attrib.get("name")

        # 'Output' structures for which data is filled in by the API are
        # marked as 'returnedonly'.
        returnedonly = True if struct.attrib.get("returnedonly") else False

        structextends = struct.attrib.get("structextends")
        structextends = structextends.split(",") if structextends else []

        members = []
        for member in struct.findall("member"):
            vk_member = VkMember.from_xml(member)
            members.append(vk_member)

        return VkStruct(name, members, returnedonly, structextends, union=union)

    @staticmethod
    def decouple_structs(structs):
        """ Helper function which decouples a list of structs.
        Structures often depend on other structures. To make the C compiler
        happy we need to define 'substructures' first. This function analyzes
        the list of structures and reorders them in such a way that they are
        decoupled.
        """

        tmp_structs = list(structs) # Don't modify the original structures.
        decoupled_structs = []

        while (len(tmp_structs) > 0):
            for struct in tmp_structs:
                dependends = False

                if not struct.required:
                    tmp_structs.remove(struct)
                    continue

                for m in struct:
                    if not (m.is_struct() or m.is_union()):
                        continue

                    # VkBaseInstructure and VkBaseOutStructure reference themselves.
                    if m.type == struct.name:
                        break

                    found = False
                    # Check if a struct we depend on has already been defined.
                    for s in decoupled_structs:
                        if s.name == m.type:
                            found = True
                            break

                    if not found:
                        # Check if the struct we depend on is even in the list of structs.
                        # If found now, it means we haven't met all dependencies before we
                        # can operate on the current struct.
                        # When generating 'host' structs we may not be able to find a struct
                        # as the list would only contain the structs requiring conversion.
                        for s in tmp_structs:
                            if s.name == m.type:
                                dependends = True
                                break

                if dependends == False:
                    decoupled_structs.append(struct)
                    tmp_structs.remove(struct)

        return decoupled_structs

    def definition(self, align=False, conv=False, postfix=None):
        """ Convert structure to textual definition.

        Args:
            align (bool, optional): enable alignment to 64-bit for win32 struct compatibility.
            conv (bool, optional): enable struct conversion if the struct needs it.
            postfix (str, optional): text to append to end of struct name, useful for struct renaming.
        """

        # Only define alias structs when doing conversions
        if self.is_alias() and not conv:
            return ""

        if self.union:
            text = "typedef union {0}".format(self.name)
        else:
            text = "typedef struct {0}".format(self.name)

        if postfix is not None:
            text += postfix

        text += "\n{\n"

        for m in self:
            text += "    {0};\n".format(m.definition())

        if postfix is not None:
            text += "}} {0}{1};\n\n".format(self.name, postfix)
        else:
            text += "}} {0};\n".format(self.name)

        for aliasee in self.aliased_by:
            text += "typedef {0} {1};\n".format(self.name, aliasee.name)

        text += "\n"

        return text

    def is_alias(self):
        return bool(self.alias)

    def add_aliased_by(self, aliasee):
        self.aliased_by.append(aliasee)

    def set_type_info(self, types):
        """ Helper function to set type information from the type registry.
        This is needed, because not all type data is available at time of
        parsing.
        """
        for m in self.members:
            type_info = types[m.type]
            m.set_type_info(type_info)

class VkRegistry(object):
    def __init__(self, reg_filename):
        # Used for storage of type information.
        self.base_types = None
        self.bitmasks = None
        self.consts = None
        self.defines = None
        self.enums = None
        self.funcpointers = None
        self.handles = None
        self.structs = None

        # We aggregate all types in here for cross-referencing.
        self.funcs = {}
        self.types = {}
        self.platforms = {}

        self.version_regex = re.compile(
            r'^'
            r'VK_VERSION_'
            r'(?P<major>[0-9])'
            r'_'
            r'(?P<minor>[0-9])'
            r'$'
        )

        # Overall strategy for parsing the registry is to first
        # parse all type / function definitions. Then parse
        # features and extensions to decide which types / functions
        # to actually 'pull in' for code generation. For each type or
        # function call we want we set a member 'required' to True.
        tree = ET.parse(reg_filename)
        root = tree.getroot()
        self._parse_enums(root)
        self._parse_types(root)
        self._parse_platforms(root)
        self._parse_commands(root)

        # Pull in any required types and functions.
        self._parse_features(root)
        self._parse_extensions(root)

        for enum in self.enums.values():
            enum.fixup_64bit_aliases()

        self._match_object_types()

        self.copyright = root.find('./comment').text

    def _is_feature_supported(self, feature):
        version = self.version_regex.match(feature)
        if not version:
            return True

        version = tuple(map(int, version.group('major', 'minor')))
        return version <= VK_VERSION

    def _mark_command_required(self, command):
        """ Helper function to mark a certain command and the datatypes it needs as required."""
        def mark_bitmask_dependencies(bitmask, types):
            if bitmask.requires is not None:
                types[bitmask.requires]["data"].required = True

        def mark_funcpointer_dependencies(fp, types):
            for m in fp.members:
                type_info = types[m.type]

                # Complex types have a matching definition e.g. VkStruct.
                # Not needed for base types such as uint32_t.
                if "data" in type_info:
                    types[m.type]["data"].required = True

        def mark_struct_dependencies(struct, types):
             for m in struct:
                type_info = types[m.type]

                # Complex types have a matching definition e.g. VkStruct.
                # Not needed for base types such as uint32_t.
                if "data" in type_info:
                    types[m.type]["data"].required = True

                if type_info["category"] == "struct" and struct.name != m.type:
                    # Yay, recurse
                    mark_struct_dependencies(type_info["data"], types)
                elif type_info["category"] == "funcpointer":
                    mark_funcpointer_dependencies(type_info["data"], types)
                elif type_info["category"] == "bitmask":
                    mark_bitmask_dependencies(type_info["data"], types)

        func = self.funcs[command]
        func.required = True

        # Pull in return type
        if func.type != "void":
            self.types[func.type]["data"].required = True

        # Analyze parameter dependencies and pull in any type needed.
        for p in func.params:
            type_info = self.types[p.type]

            # Check if we are dealing with a complex type e.g. VkEnum, VkStruct and others.
            if "data" not in type_info:
                continue

            # Mark the complex type as required.
            type_info["data"].required = True
            if type_info["category"] == "struct":
                struct = type_info["data"]
                mark_struct_dependencies(struct, self.types)
            elif type_info["category"] == "bitmask":
                mark_bitmask_dependencies(type_info["data"], self.types)

    def _match_object_types(self):
        """ Matches each handle with the correct object type. """
        # Use upper case comparison for simplicity.
        object_types = {}
        for value in self.enums["VkObjectType"].values:
            object_name = "VK" + value.name[len("VK_OBJECT_TYPE"):].replace("_", "")
            object_types[object_name] = value.name

        for handle in self.handles:
            if not handle.is_required():
                continue
            handle.object_type = object_types.get(handle.name.upper())
            if not handle.object_type:
                LOGGER.warning("No object type found for {}".format(handle.name))

    def _parse_commands(self, root):
        """ Parse command section containing the Vulkan function calls. """
        funcs = {}
        commands = root.findall("./commands/")

        # As of Vulkan 1.1, various extensions got promoted to Core.
        # The old commands (e.g. KHR) are available for backwards compatibility
        # and are marked in vk.xml as 'alias' to the non-extension type.
        # The registry likes to avoid data duplication, so parameters and other
        # metadata need to be looked up from the Core command.
        # We parse the alias commands in a second pass.
        alias_commands = []
        for command in commands:
            alias_name = command.attrib.get("alias")
            if alias_name:
                alias_commands.append(command)
                continue

            func = VkFunction.from_xml(command, self.types)
            funcs[func.name] = func

        for command in alias_commands:
            alias_name = command.attrib.get("alias")
            alias = funcs[alias_name]
            func = VkFunction.from_alias(command, alias)
            funcs[func.name] = func

        # To make life easy for the code generation, separate all function
        # calls out in the 4 types of Vulkan functions:
        # device, global, physical device and instance.
        device_funcs = []
        global_funcs = []
        phys_dev_funcs = []
        instance_funcs = []
        for func in funcs.values():
            if func.is_device_func():
                device_funcs.append(func)
            elif func.is_global_func():
                global_funcs.append(func)
            elif func.is_phys_dev_func():
                phys_dev_funcs.append(func)
            else:
                instance_funcs.append(func)

        # Sort function lists by name and store them.
        self.device_funcs = sorted(device_funcs, key=lambda func: func.name)
        self.global_funcs = sorted(global_funcs, key=lambda func: func.name)
        self.phys_dev_funcs = sorted(phys_dev_funcs, key=lambda func: func.name)
        self.instance_funcs = sorted(instance_funcs, key=lambda func: func.name)

        # The funcs dictionary is used as a convenient way to lookup function
        # calls when needed e.g. to adjust member variables.
        self.funcs = OrderedDict(sorted(funcs.items()))

    def _parse_enums(self, root):
        """ Parse enums section or better described as constants section. """
        enums = {}
        self.consts = []
        for enum in root.findall("./enums"):
            name = enum.attrib.get("name")
            _type = enum.attrib.get("type")

            if _type in ("enum", "bitmask"):
                enums[name] = VkEnum.from_xml(enum)
            else:
                # If no type is set, we are dealing with API constants.
                for value in enum.findall("enum"):
                    # If enum is an alias, set the value to the alias name.
                    # E.g. VK_LUID_SIZE_KHR is an alias to VK_LUID_SIZE.
                    alias = value.attrib.get("alias")
                    if alias:
                        self.consts.append(VkConstant(value.attrib.get("name"), alias))
                    else:
                        self.consts.append(VkConstant(value.attrib.get("name"), value.attrib.get("value")))

        self.enums = OrderedDict(sorted(enums.items()))

    def _process_require_enum(self, enum_elem, ext=None, only_aliased=False):
        if "extends" in enum_elem.keys():
            enum = self.types[enum_elem.attrib["extends"]]["data"]

            # Need to define VkEnumValues which were aliased to by another value. This is necessary
            # from VK spec version 1.2.135 where the provisional VK_KHR_ray_tracing extension was
            # added which altered VK_NV_ray_tracing's VkEnumValues to alias to the provisional
            # extension.
            aliased = False
            for _, t in self.types.items():
                if t["category"] != "enum":
                    continue
                if not t["data"]:
                    continue
                for value in t["data"].values:
                    if value.alias == enum_elem.attrib["name"]:
                        aliased = True

            if only_aliased and not aliased:
                return

            if "bitpos" in enum_elem.keys():
                # We need to add an extra value to an existing enum type.
                # E.g. VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_CUBIC_BIT_IMG to VkFormatFeatureFlagBits.
                enum.create_bitpos(enum_elem.attrib["name"], int(enum_elem.attrib["bitpos"]))

            elif "offset" in enum_elem.keys():
                # Extensions promoted to Core, have the extension number as part
                # of the enum value. Else retrieve from the extension tag.
                if enum_elem.attrib.get("extnumber"):
                    ext_number = int(enum_elem.attrib.get("extnumber"))
                else:
                    ext_number = int(ext.attrib["number"])
                offset = int(enum_elem.attrib["offset"])
                value = EXT_BASE + (ext_number - 1) * EXT_BLOCK_SIZE + offset

                # Deal with negative values.
                direction = enum_elem.attrib.get("dir")
                if direction is not None:
                    value = -value

                enum.create_value(enum_elem.attrib["name"], str(value))

            elif "value" in enum_elem.keys():
                enum.create_value(enum_elem.attrib["name"], enum_elem.attrib["value"])
            elif "alias" in enum_elem.keys():
                enum.create_alias(enum_elem.attrib["name"], enum_elem.attrib["alias"])

        elif "value" in enum_elem.keys():
            # Constant with an explicit value
            if only_aliased:
                return

            self.consts.append(VkConstant(enum_elem.attrib["name"], enum_elem.attrib["value"]))
        elif "alias" in enum_elem.keys():
            # Aliased constant
            if not only_aliased:
                return

            self.consts.append(VkConstant(enum_elem.attrib["name"], enum_elem.attrib["alias"]))

    @staticmethod
    def _require_type(type_info):
        if type_info.is_alias():
            type_info = type_info.alias
        type_info.required = True
        if type(type_info) == VkStruct:
            for member in type_info.members:
                if "data" in member.type_info:
                  VkRegistry._require_type(member.type_info["data"])

    def _parse_extensions(self, root):
        """ Parse extensions section and pull in any types and commands for this extension. """
        extensions = []
        exts = root.findall("./extensions/extension")
        deferred_exts = []
        skipped_exts = []

        def process_ext(ext, deferred=False):
            ext_name = ext.attrib["name"]

            # Set extension name on any functions calls part of this extension as we
            # were not aware of the name during initial parsing.
            commands = ext.findall("require/command")
            for command in commands:
                cmd_name = command.attrib["name"]
                # Need to verify that the command is defined, and otherwise skip it.
                # vkCreateScreenSurfaceQNX is declared in <extensions> but not defined in
                # <commands>. A command without a definition cannot be enabled, so it's valid for
                # the XML file to handle this, but because of the manner in which we parse the XML
                # file we pre-populate from <commands> before we check if a command is enabled.
                if cmd_name in self.funcs:
                    self.funcs[cmd_name].extensions.append(ext_name)

            # Enum
            vk_types = ext.findall("require/type")
            for vk_type in vk_types:
                type_name = vk_type.attrib["name"]
                if type_name in self.enums:
                    self.enums[type_name].extensions.append(ext_name)

            # Some extensions are not ready or have numbers reserved as a place holder.
            if ext.attrib["supported"] == "disabled":
                LOGGER.debug("Skipping disabled extension: {0}".format(ext_name))
                skipped_exts.append(ext_name)
                return

            # Defer extensions with 'sortorder' as they are order-dependent for spec-parsing.
            if not deferred and "sortorder" in ext.attrib:
                deferred_exts.append(ext)
                return

            # Disable highly experimental extensions as the APIs are unstable and can
            # change between minor Vulkan revisions until API is final and becomes KHR
            # or NV.
            if "KHX" in ext_name or "NVX" in ext_name:
                LOGGER.debug("Skipping experimental extension: {0}".format(ext_name))
                skipped_exts.append(ext_name)
                return

            # Extensions can define VkEnumValues which alias to provisional extensions. Pre-process
            # extensions to define any required VkEnumValues before the platform check below.
            for require in ext.findall("require"):
                # Extensions can add enum values to Core / extension enums, so add these.
                for enum_elem in require.findall("enum"):
                    self._process_require_enum(enum_elem, ext, only_aliased=True)

            if "requires" in ext.attrib:
                # Check if this extension builds on top of another unsupported extension.
                requires = ext.attrib["requires"].split(",")
                if len(set(requires).intersection(skipped_exts)) > 0:
                    skipped_exts.append(ext_name)
                    return

            LOGGER.debug("Loading extension: {0}".format(ext_name))

            # Extensions can define one or more require sections each requiring
            # different features (e.g. Vulkan 1.1). Parse each require section
            # separately, so we can skip sections we don't want.
            for require in ext.findall("require"):
                # Extensions can add enum values to Core / extension enums, so add these.
                for enum_elem in require.findall("enum"):
                    self._process_require_enum(enum_elem, ext)

                for t in require.findall("type"):
                    type_info = self.types[t.attrib["name"]]["data"]
                    self._require_type(type_info)
                feature = require.attrib.get("feature")
                if feature and not self._is_feature_supported(feature):
                    continue

                # Pull in any commands we need. We infer types to pull in from the command
                # as well.
                for command in require.findall("command"):
                    cmd_name = command.attrib["name"]
                    self._mark_command_required(cmd_name)


            # Store a list with extensions.
            ext_info = {"name" : ext_name, "platform": ext.attrib.get("platform"), "type" : ext.attrib["type"]}
            extensions.append(ext_info)


        # Process extensions, allowing for sortorder to defer extension processing
        for ext in exts:
            process_ext(ext)

        deferred_exts.sort(key=lambda ext: ext.attrib["sortorder"])

        # Respect sortorder
        for ext in deferred_exts:
            process_ext(ext, deferred=True)

        # Sort in alphabetical order.
        self.extensions = sorted(extensions, key=lambda ext: ext["name"])

    def _parse_features(self, root):
        """ Parse the feature section, which describes Core commands and types needed. """

        for feature in root.findall("./feature"):
            feature_name = feature.attrib["name"]
            for require in feature.findall("require"):
                LOGGER.info("Including features for {0}".format(require.attrib.get("comment")))
                for tag in require:
                    if tag.tag == "comment":
                        continue
                    elif tag.tag == "command":
                        if not self._is_feature_supported(feature_name):
                            continue
                        name = tag.attrib["name"]
                        self._mark_command_required(name)
                    elif tag.tag == "enum":
                        self._process_require_enum(tag)
                    elif tag.tag == "type":
                        name = tag.attrib["name"]

                        # Skip pull in for vk_platform.h for now.
                        if name == "vk_platform":
                            continue

                        type_info = self.types[name]
                        type_info["data"].required = True

    def _parse_platforms(self, root):
        platforms = root.findall("./platforms/platform")
        for p in platforms:
            platform_info = {}
            platform_info["name"] = p.attrib.get("name", None)
            platform_info["protect"] = p.attrib.get("protect", None)

            self.platforms[platform_info["name"]] = platform_info

    def _parse_types(self, root):
        """ Parse types section, which contains all data types e.g. structs, typedefs etcetera. """
        types = root.findall("./types/type")

        base_types = []
        bitmasks = []
        defines = []
        funcpointers = []
        handles = []
        structs = []

        alias_types = []
        for t in types:
            type_info = {}
            type_info["category"] = t.attrib.get("category", None)
            type_info["requires"] = t.attrib.get("requires", None)

            # We parse aliases in a second pass when we know more.
            alias = t.attrib.get("alias")
            if alias:
                LOGGER.debug("Alias found: {0}".format(alias))
                alias_types.append(t)
                continue

            if type_info["category"] in ["include"]:
                continue

            if type_info["category"] == "basetype":
                name = t.find("name").text
                _type = None
                if not t.find("type") is None:
                    _type = t.find("type").text
                basetype = VkBaseType(name, _type)
                base_types.append(basetype)
                type_info["data"] = basetype

            # Basic C types don't need us to define them, but we do need data for them
            if type_info["requires"] == "vk_platform":
                requires = type_info["requires"]
                basic_c = VkBaseType(name, _type, requires=requires)
                type_info["data"] = basic_c

            if type_info["category"] == "bitmask":
                name = t.find("name").text
                _type = t.find("type").text

                # Most bitmasks have a requires attribute used to pull in
                # required '*FlagBits" enum.
                requires = type_info["requires"]
                bitmask = VkBaseType(name, _type, requires=requires)
                bitmasks.append(bitmask)
                type_info["data"] = bitmask

            if type_info["category"] == "define":
                define = VkDefine.from_xml(t)
                defines.append(define)
                type_info["data"] = define

            if type_info["category"] == "enum":
                name = t.attrib.get("name")
                # The type section only contains enum names, not the actual definition.
                # Since we already parsed the enum before, just link it in.
                try:
                    type_info["data"] = self.enums[name]
                except KeyError as e:
                    # Not all enums seem to be defined yet, typically that's for
                    # ones ending in 'FlagBits' where future extensions may add
                    # definitions.
                    type_info["data"] = None

            if type_info["category"] == "funcpointer":
                funcpointer = VkFunctionPointer.from_xml(t)
                funcpointers.append(funcpointer)
                type_info["data"] = funcpointer

            if type_info["category"] == "handle":
                handle = VkHandle.from_xml(t)
                handles.append(handle)
                type_info["data"] = handle

            if type_info["category"] in ["struct", "union"]:
                # We store unions among structs as some structs depend
                # on unions. The types are very similar in parsing and
                # generation anyway. The official Vulkan scripts use
                # a similar kind of hack.
                struct = VkStruct.from_xml(t)
                structs.append(struct)
                type_info["data"] = struct

            # Name is in general within a name tag else it is an optional
            # attribute on the type tag.
            name_elem = t.find("name")
            if name_elem is not None:
                type_info["name"] = name_elem.text
            else:
                type_info["name"] = t.attrib.get("name", None)

            # Store all type data in a shared dictionary, so we can easily
            # look up information for a given type. There are no duplicate
            # names.
            self.types[type_info["name"]] = type_info

        # Second pass for alias types, so we can retrieve all data from
        # the aliased object.
        for t in alias_types:
            type_info = {}
            type_info["category"] = t.attrib.get("category")
            type_info["name"] = t.attrib.get("name")

            alias = t.attrib.get("alias")

            if type_info["category"] == "bitmask":
                bitmask = VkBaseType(type_info["name"], alias, alias=self.types[alias]["data"])
                bitmasks.append(bitmask)
                type_info["data"] = bitmask

            if type_info["category"] == "enum":
                enum = VkEnum.from_alias(t, self.types[alias]["data"])
                type_info["data"] = enum
                self.enums[enum.name] = enum

            if type_info["category"] == "handle":
                handle = VkHandle.from_alias(t, self.types[alias]["data"])
                handles.append(handle)
                type_info["data"] = handle

            if type_info["category"] == "struct":
                struct = VkStruct.from_alias(t, self.types[alias]["data"])
                structs.append(struct)
                type_info["data"] = struct

            self.types[type_info["name"]] = type_info

        # We need detailed type information during code generation
        # on structs for alignment reasons. Unfortunately structs
        # are parsed among other types, so there is no guarantee
        # that any types needed have been parsed already, so set
        # the data now.
        for struct in structs:
            struct.set_type_info(self.types)

            # Alias structures have enum values equivalent to those of the
            # structure which they are aliased against. we need to ignore alias
            # structs when populating the struct extensions list, otherwise we
            # will create duplicate case entries.
            if struct.alias:
                continue

            for structextend in struct.structextends:
                s = self.types[structextend]["data"]
                s.struct_extensions.append(struct)

        # Guarantee everything is sorted, so code generation doesn't have
        # to deal with this.
        self.base_types = sorted(base_types, key=lambda base_type: base_type.name)
        self.bitmasks = sorted(bitmasks, key=lambda bitmask: bitmask.name)
        self.defines = defines
        self.enums = OrderedDict(sorted(self.enums.items()))
        self.funcpointers = sorted(funcpointers, key=lambda fp: fp.name)
        self.handles = sorted(handles, key=lambda handle: handle.name)
        self.structs = sorted(structs, key=lambda struct: struct.name)
