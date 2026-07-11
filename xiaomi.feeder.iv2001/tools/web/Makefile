.PHONY: test-web preview-web

test-web:
	node --test test_logic.mjs test_mock_api.mjs

preview-web:
	@bytes=$$(./build.sh 2>&1 | sed -n 's/.*(\([0-9]*\) bytes gzipped.*/\1/p'); \
	WEB_UI_GZ_BYTES=$$bytes node dev_server.mjs
