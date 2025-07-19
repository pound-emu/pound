# Compiling Pound (incomplete documentation!!)

# To use and compile Pound, you will need to **dump your own Switch or Switch 2 firmware** and place it in the following directory:

```
/assets/fw/
```

# No firmware or system files are provided with this emulator.

## Windows (10/11/vNext, Server 2016/2019/20h2/21h2/2022/22h2/23h2/24h2/2025/vNext)
(soon)

## Linux (Fedora/RHEL/openSUSE)

## Linux (Debian/Ubuntu)

### Installing dependencies and cloning Pound

Run the following in your terminal:

```bash
sudo apt update

sudo apt install -y ninja-build clang lld cmake ccache libx11-dev libxext-dev libxrandr-dev libxcursor-dev libxi-dev libxinerama-dev libwayland-dev libxkbcommon-dev wayland-protocols git
```

Now run the following:

```bash
git clone https://www.github.com/pound-emu/pound.git
cd pound
```

Initialize the submodules:

```bash
cd 3rd_Party
git submodule update --init --recursive
cd ..
```
and then:

`cmake -G Ninja -B . -DCMAKE_BUILD_TYPE=Release -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++`

## Linux (Arch/SteamOS)

Run the following commands in your terminal:

```bash
sudo pacman -S ninja clang lld cmake ccache libx11 libxext libxrandr libxcursor libxi libxinerama libxkbcommon wayland-protocols git

```

Now:

```bash
git clone https://www.github.com/pound-emu/pound.git
```

then:

```bash
cd pound
```

To initialize the submodules, direct to ``3rd_Party`` and than run ``git submodule update --init --recursive``

```bash
cd 3rd_Party
git submodule update --init --recursive
cd ..
```

lastly (in the pound directory not 3rd_Party) run:

```bash
cmake -G Ninja -B . -DCMAKE_BUILD_TYPE=Release -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++
```
