.PHONY: clean All

All:
	@echo "----------Building project:[ S3ORAM - Debug ]----------"
	@cd "S3ORAM" && "$(MAKE)" -f  "S3ORAM.mk"
clean:
	@echo "----------Cleaning project:[ S3ORAM - Debug ]----------"
	@cd "S3ORAM" && "$(MAKE)" -f  "S3ORAM.mk" clean
