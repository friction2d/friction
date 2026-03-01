Name: friction
Version: __FRICTION_PKG_VERSION__
Release: 1
Summary: Motion graphics and animation
Group: System Environment/Base
License: GPL-3.0-only
Source0: friction-__FRICTION_VERSION__.tar

AutoReq: no
%global debug_package %{nil}
%global __provides_exclude_from /opt
%global __os_install_post %{nil}

%description
Powerful and versatile motion graphics and animation application that allows you to create vector and raster animations for web and video.

%prep
%setup -n friction-__FRICTION_VERSION__

%build

%install

cp -rfa opt %{buildroot}/
cp -rfa usr %{buildroot}/

%files
%defattr(-,root,root,-)
/opt/friction
/usr/bin/friction
/usr/share/applications/__APPID__.desktop
/usr/share/metainfo/__APPID__.appdata.xml
/usr/share/mime/packages/__APPID__.xml
/usr/share/icons/hicolor

%changelog

