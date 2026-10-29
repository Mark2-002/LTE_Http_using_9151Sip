# LTE HTTP using 9151 SIP

This project demonstrates how to connect an nRF9151-based device to the internet using LTE Cat-M/NB-IoT and send HTTP requests to a remote server. The implementation is based on Zephyr RTOS and targets the `thingy91x/nrf9151/ns` board, which is the board used during validation for this example.

## Overview

The application initializes the modem, connects to the LTE network, resolves the configured HTTP host, and sends HTTP requests such as:

- POST JSON payloads
- POST plain text payloads
- PUT requests
- GET requests

This repo is intended for testing and validating LTE connectivity and HTTP communication over a Jio NB-IoT connection, with IPv6/DNS fallback handling included for compatibility.

## Features

- LTE modem initialization with `nrf_modem_lib`
- Automatic LTE network registration handling
- DNS fallback configuration for IPv6-only network conditions
- HTTP client requests using Zephyr networking APIs
- JSON and text payload examples
- Server communication tested with a public webhook endpoint

## Project structure

The application is split into two main source files:

- `src/main.c` — modem setup, network registration flow, and application loop
- `src/http.c` — socket and HTTP request logic for GET/POST/PUT operations

The build configuration is controlled by the Zephyr project files:

- `CMakeLists.txt`
- `Kconfig`
- `prj.conf`

## Configuration

The main runtime configuration is in `prj.conf`.

Key settings include:

- LTE modem library enabled
- networking enabled with socket support
- HTTP client enabled
- DNS fallback enabled for IPv6-only network access
- HTTP host and path configured through `CONFIG_HTTP_SAMPLE_HOSTNAME` and `CONFIG_HTTP_URL`

Important note: the sample uses a public webhook URL by default. This may expire over time, so it is recommended to update the host and URL in `prj.conf` before use.

## Build instructions

Use the Zephyr build system with the following setup:

1. Use the nRF Connect SDK version `3.1.1`
2. Use the matching toolchain version `3.1.1`
3. Select the board target: `thingy91x/nrf9151/ns`
4. Use `prj.conf` as the project configuration
5. Generate and build the firmware

This is the build flow described in the repository's build instructions.

## Notes

- The implementation is validated on the `thingy91x/nrf9151/ns` board.
- The default server endpoint may need to be replaced if it expires.
- In some cases, if LTE connectivity is unstable, the DNS fallback address may need to be adjusted.
- The repository includes a comment indicating this fallback was added to work around IPv6-only issues seen with Jio NB-IoT SIMs.

## Author

Atul V (`MARK2-002`)
