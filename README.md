WARTHOG JANUSHASH MINER
Copyright (c) 2023 CoinFuMasterShifu, Pumbaa, Timon & Rafiki
<p align="center">
  <img src="doc/img/warthog_logo.png" style="width:300px;"/>
</p>



## 📦 What is in the box?

* GPU/CPU Miner for Janushash

## 💻 System Requirements

* Linux
* gcc11 or newer
* meson
* ninja
* opencl

Note: Ubuntu 20.04 does not meet these requirements.

## 😵‍💫 BUILD INSTRUCTIONS

* Install gcc, meson, ninja: apt install meson ninja-build build-essential
* Clone the repo: `git clone https://github.com/CoinFuMasterShifu/janusminer`
* cd into the repo: `cd janusminer`
* Create build directory: `meson build .` (`meson build . --buildtype=release` for better performance)
* cd into build directory: `cd build`
* [Optional] For old OpenCL headers (like on Ubuntu 20.04):
  - `meson configure -Dopencl-legacy=true`
* Compile using ninja: `ninja`

### Docker build (node and wallet)
#### System Requirements

* Linux
* Docker

#### Build for Linux

##### Ubuntu 18.04
* Run `DOCKER_BUILDKIT=1 docker build . -f Dockerfile_Ubuntu18 --output build` in the repo directory.
##### Ubuntu 20.04
* Run `DOCKER_BUILDKIT=1 docker build . -f Dockerfile_Ubuntu20 --output build` in the repo directory.
##### Ubuntu 22.04
* Run `DOCKER_BUILDKIT=1 docker build . -f Dockerfile_Ubuntu22 --output build` in the repo directory.
Binaries are located in `./build` directory.


## ▶️ USAGE

* Linux only at the moment
* Compile with meson/ninja
* Run the miner (use some restarter in case it crashes)
* In case you are unsure, things should work exactly as in [this Warthog node guide](https://github.com/warthog-network/warthog-guide)
