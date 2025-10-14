################################################################################
# \file common.mk
# \version 1.0
#
# \brief
# Settings shared across all projects.
#
################################################################################
# \copyright
# (c) 2023-2025, Infineon Technologies AG, or an affiliate of Infineon
# Technologies AG.  SPDX-License-Identifier: Apache-2.0
# 
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
# 
#     http://www.apache.org/licenses/LICENSE-2.0
# 
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
################################################################################

MTB_TYPE=PROJECT

# Target board/hardware (BSP).
# To change the target, it is recommended to use the Library manager
# ('make library-manager' from command line), which will also update 
# Eclipse IDE launch configurations.
TARGET=KIT_PSE84_EVAL_EPC2

# Name of toolchain to use. Options include:
#
# GCC_ARM -- GCC provided with ModusToolbox software
# ARM     -- ARM Compiler (must be installed separately)
# IAR     -- IAR Compiler (must be installed separately)
# LLVM_ARM	-- LLVM Embedded Toolchain (must be installed separately)
#
# See also: CY_COMPILER_PATH below
TOOLCHAIN=GCC_ARM

#For IAR/ARM/LLVM_ARM; uncomment the lines below to add path to the compiler (Note: Update the path as required)
#CY_COMPILER_IAR_DIR=C:/Program Files/IAR Systems/Embedded Workbench 9.2/arm
#CY_COMPILER_ARM_DIR=C:/Program Files/ArmCompilerforEmbedded6.22
#CY_COMPILER_LLVM_ARM_DIR=C:/Program Files/LLVM-ET-Arm-19.1.5-Windows-x86_64

# Default build configuration. Options include:
#
# Debug -- build with minimal optimizations, focus on debugging.
# Release -- build with full optimizations
# Custom -- build with custom configuration, set the optimization flag in CFLAGS
# 
# If CONFIG is manually edited, ensure to update or regenerate 
# launch configurations for your IDE.
CONFIG?=Debug

#Python path
#CY_PYTHON_PATH=C:/Python/Python312/python.exe

#Image Type can be BOOT or UPDATE. Default to BOOT.
# To overwrite default values, set IMG_TYPE?=UPDATE here or pass IMG_TYPE=UPDATE in commandline
IMG_TYPE?=BOOT

# This code examples demonstartes update by 'overwrite' only.
UPDATE_TYPE?=overwrite

#Sets the names of the application for OTA  
APP_1_NAME=proj_cm33_s
APP_2_NAME=proj_cm33_ns
APP_3_NAME=proj_cm55
    
#Image version for Boot usecase
ifeq ($(IMG_TYPE),BOOT)
# JSON file genearted by bootloader personality 
# COMBINE_SIGN_JSON?=./bsps/TARGET_$(TARGET)/config/GeneratedSource/boot_with_bldr.json

#Sets the version of the application for OTA    
APP_VERSION_MAJOR?=1
APP_VERSION_MINOR?=0
APP_VERSION_BUILD?=0

#Image version for update usecase
else ifeq ($(IMG_TYPE),UPDATE)

ifeq ($(UPDATE_TYPE),overwrite)
## Following 
COMBINE_SIGN_JSON?=./configs/update_with_epb_overwrite.json
else
$(error Invalid UPDATE_TYPE. This code example demonstartes only 'overwrite' update)
endif

#Sets the version of the application for OTA    
APP_VERSION_MAJOR?=1
APP_VERSION_MINOR?=1
APP_VERSION_BUILD?=0

else
$(error Invalid IMG_TYPE. Please set it to either BOOT or UPDATE)
endif

# Building ifx-mcuboot with ARM compiler requries some
# specific symbols. However, linking when ifx-mcuboot is 
# added as a library to an existing project,
# those symbols are not required. Excluding those symbols, 
#which are not needed for this application build
ifeq ($(TOOLCHAIN),ARM)
  DEFINES+=MCUBOOT_SKIP_CLEANUP_RAM=1
endif

include ../common_app.mk
