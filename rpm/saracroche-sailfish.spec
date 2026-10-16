Name:       saracroche-sailfish
Summary:    Blocks French telemarketing calls with the Saracroche lists (unofficial port)
Version:    0.1.0
Release:    1
License:    GPLv3
URL:        https://saracroche.org
Source0:    %{name}-%{version}.tar.bz2

Requires:   sailfishsilica-qt5 >= 0.10.9
Requires:   voicecall-qt5-plugin-voicecall-filter >= 0.8.8
Requires:   mlite-qt5 >= 0.5.0
BuildRequires:  pkgconfig(sailfishapp) >= 1.0.2
BuildRequires:  pkgconfig(Qt5Core)
BuildRequires:  pkgconfig(Qt5Qml)
BuildRequires:  pkgconfig(Qt5Quick)
BuildRequires:  pkgconfig(Qt5Network)
BuildRequires:  pkgconfig(Qt5Sql)
BuildRequires:  pkgconfig(mlite5) >= 0.5.0
BuildRequires:  desktop-file-utils

%description
Unofficial Sailfish OS port of Saracroche. Downloads the Saracroche lists of
French telemarketing numbers every day and feeds them to the native Sailfish
call filter, which rejects those calls.

%prep
%setup -q -n %{name}-%{version}

%build
%qmake5
%make_build

%install
%qmake5_install
desktop-file-install --delete-original --dir %{buildroot}%{_datadir}/applications \
    %{buildroot}%{_datadir}/applications/*.desktop

%files
%license LICENSE
%{_bindir}/%{name}
%{_datadir}/%{name}
%{_datadir}/applications/%{name}.desktop
%{_datadir}/icons/hicolor/*/apps/%{name}.png
%{_userunitdir}/saracroche-sync.service
%{_userunitdir}/saracroche-sync.timer
