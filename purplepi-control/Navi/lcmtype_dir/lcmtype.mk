################################################################################
# lcmtype.mk
################################################################################
# set compail dir
MOD_DIR_NAME = lcmtype_dir
MOD_SRC_DIR = $(KERNEL_TOP_DIR)/$(MOD_DIR_NAME)
MOD_OBJ_DIR = ./project/$(MOD_DIR_NAME)

# get files table
ROBOT_KERNEL_OBJ += $(addprefix $(MOD_OBJ_DIR)/,$(subst .c,.o,$(wildcard $(MOD_SRC_DIR)/*.c)))
ROBOT_KERNEL_FILE_DIR += $(wildcard $(MOD_SRC_DIR)/*.*~)

# compail
$(MOD_OBJ_DIR)/%.o: $(MOD_SRC_DIR)/%.c
	@echo 'Compail... $<'
	$(CC) -c $< -o $@ $(C_OPTIMIZATION) $(C_FLAG) $(USR_DEF)
	@echo ' '

#$(MOD_OBJ_DIR)/%.o: $(MOD_SRC_DIR)/%.c
#	@echo 'Compail... $<'
#	$(CC) -c $< -o $@ $(C_OPTIMIZATION) $(C_FLAG) $(USR_DEF)
#	@echo ' '
