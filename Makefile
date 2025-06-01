TARGET_FILES =  $(wildcard src/*.c)
OUT_DIR=bin

all: bot

bot: $(TARGET_FILES)
	mkdir -p $(OUT_DIR)
	gcc -Wall -O3 -g $(TARGET_FILES) -o $(OUT_DIR)/bot

clean:
	rm -f $(OUT_DIR)/bot