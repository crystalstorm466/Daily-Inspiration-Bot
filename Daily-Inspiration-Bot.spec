me:       daily-inspiration-bot
Version:    1.0.0
Release:    1%{?dist}
Summary:    A simple bot to provide daily inspirational quotes

License:    MIT
URL:        https://github.com/crystalstorm466/Daily-Inspiration-Bot
Source0:    %{url}/archive/v%{version}/%{name}-%{version}.tar.gz

BuildRequires:  cmake
BuildRequires:  make
BuildRequires:  gcc-c++
BuildRequires:  dpp-devel
BuildRequires:  openssl-devel
BuildRequires:  nlohmann-json-devel
BuildRequires:  libcurl-devel
BuildRequires:  libstdc++-static
BuildRequires:  libatomic

%description
The Daily-Inspiration-Bot is a simple C++ program that connects to Discord
and retrieves daily inspirational quotes from the Inspirobot API.
It is written using the D++ library and uses a `subscribed_channels.txt` file
to manage which channels it posts to.

%prep
%setup -q

%build
%cmake -DBUILD_SHARED_LIBS=OFF
%cmake_build

%install
%cmake_install

%files
%license LICENSE
%{_bindir}/discord-bot
