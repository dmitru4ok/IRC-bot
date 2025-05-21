TARGET_FILES =  $(wildcard src/*.c)
OUT_DIR=bin

all: bot

bot: $(TARGET_FILES)
	gcc -Wall -O3 $(TARGET_FILES) -o $(OUT_DIR)/bot

clean:
	rm -f $(OUT_DIR)/bot