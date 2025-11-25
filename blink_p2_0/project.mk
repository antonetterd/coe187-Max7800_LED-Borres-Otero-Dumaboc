# -----------------------------------------------------------------------------
# Project Name
# -----------------------------------------------------------------------------
PROJ_NAME = blink_p2_0

# -----------------------------------------------------------------------------
# Path to root of Maxim SDK directory
# Current folder: C:/MaximSDK/Examples/MAX78000/blink_p2_0
#   ..      -> C:/MaximSDK/Examples/MAX78000
#   ../..   -> C:/MaximSDK/Examples
#   ../../..-> C:/MaximSDK   ✅ (SDK root)
# -----------------------------------------------------------------------------
ifeq "$(MAXIM_PATH)" ""
MAXIM_PATH = ../../..
endif

# -----------------------------------------------------------------------------
# Select the board you are using
# -----------------------------------------------------------------------------
ifeq "$(BOARD)" ""
BOARD = FTHR_RevA
#BOARD = EvKit_V1
endif

# (Optional) extra flags for this project
# PROJ_CFLAGS += -DSOME_DEFINE
