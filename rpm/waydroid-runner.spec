Name:       waydroid-runner

%{!?qtc_qmake:%define qtc_qmake %qmake}
%{!?qtc_qmake5:%define qtc_qmake5 %qmake5}
%{!?qtc_make:%define qtc_make make}
%{?qtc_builddir:%define _builddir %qtc_builddir}
Summary:    Waydroid session wrapper
Version:    0.8.0
Release:    1
Group:      Qt/Qt
License:    BSD-3
URL:        https://github.com/sailfishos-sony-pdx246/waydroid-runner
Source0:    %{name}-%{version}.tar.bz2
Requires:   sailfishsilica-qt5 >= 0.10.9
# id.waydro.Initializer.Remove (Settings > Apps > Waydroid: Remove Android)
Requires:   waydroid >= 1.6.3-9
# invoker and booster-generic start the background instance
Requires:   mapplauncherd
# Settings > Apps > Waydroid
Requires:   nemo-qml-plugin-configuration-qt5
BuildRequires:  pkgconfig(sailfishapp) >= 1.0.2
BuildRequires:  pkgconfig(Qt5Core)
BuildRequires:  pkgconfig(Qt5Qml)
BuildRequires:  pkgconfig(Qt5Quick)
BuildRequires:  pkgconfig(Qt5Compositor) >= 5.6.0
BuildRequires:  pkgconfig(Qt5Svg)
BuildRequires:  pkgconfig(Qt5DBus)
BuildRequires:  pkgconfig(mlite5)
BuildRequires:  qt5-qttools-linguist

%description
Wrapper for running Waydroid session on Sailfish. Closing its window
keeps Android running in the background; the launcher shows it again.
Android apps' launchers and notification taps open them in its window.
Settings > Apps > Waydroid sets whether closing the window stops Android,
and removes Android with its apps and data, which frees its space.
At the first start it sets Android up through Waydroid's own setup, which
downloads the images of /usr/share/waydroid-extra/channels.cfg's channels.

PackageName: Waydroid Runner
Type: desktop-application
Categories:
 - System
 - Video
Custom:
 - Repo: https://github.com/sailfishos-sony-pdx246/waydroid-runner
Icon: https://raw.githubusercontent.com/sailfishos-open/waydroid-runner/master/icons/256x256/waydroid-runner.png
Screenshots:
 - https://raw.githubusercontent.com/sailfishos-open/waydroid-runner/master/screenshots/waiting-start.png
 - https://raw.githubusercontent.com/sailfishos-open/waydroid-runner/master/screenshots/lineage.png
 - https://raw.githubusercontent.com/sailfishos-open/waydroid-runner/master/screenshots/android-version.png

%prep
%setup -q -n %{name}-%{version}

%build
%qtc_qmake5 ARCH=%{_arch} VERSION='%{version}-%{release}'
%qtc_make %{?_smp_mflags}
# The settings page's QML plugin, a project of its own
cd plugin
%qtc_qmake5
%qtc_make %{?_smp_mflags}
cd ..

%install
rm -rf %{buildroot}
%qmake5_install
make -C plugin install INSTALL_ROOT=%{buildroot}

desktop-file-install --delete-original       \
  --dir %{buildroot}%{_datadir}/applications             \
   %{buildroot}%{_datadir}/applications/*.desktop

%files
%defattr(-,root,root,-)
%license LICENSE
%{_bindir}/%{name}
%{_datadir}/%{name}
%{_datadir}/applications/%{name}.desktop
%{_datadir}/icons/hicolor/*/apps/%{name}.png
%{_prefix}/lib/systemd/user/%{name}.service
%{_datadir}/dbus-1/services/org.sailfishosopen.WaydroidRunner.service
%{_datadir}/jolla-settings/entries/%{name}.json
%{_datadir}/jolla-settings/pages/%{name}
%{_libdir}/qt5/qml/org/sailfishosopen/waydroidrunner
