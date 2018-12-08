.PHONY: clean All

All:
	@echo "----------Building project:[ libshmutil - Debug ]----------"
	@cd "libshmutil" && "$(MAKE)" -f  "libshmutil.mk"
clean:
	@echo "----------Cleaning project:[ libshmutil - Debug ]----------"
	@cd "libshmutil" && "$(MAKE)" -f  "libshmutil.mk" clean
