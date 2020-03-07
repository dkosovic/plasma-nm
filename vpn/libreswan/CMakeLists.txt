add_definitions(-DTRANSLATION_DOMAIN=\"plasmanetworkmanagement_libreswanui\")

set(libreswan_SRCS
    libreswan.cpp
    libreswanwidget.cpp
    libreswanauth.cpp
)

ki18n_wrap_ui(libreswan_SRCS libreswan.ui libreswanauth.ui)

add_library(plasmanetworkmanagement_libreswanui ${libreswan_SRCS})

kcoreaddons_desktop_to_json(plasmanetworkmanagement_libreswanui plasmanetworkmanagement_libreswanui.desktop)

target_link_libraries(plasmanetworkmanagement_libreswanui
    plasmanm_internal
    plasmanm_editor
    KF5::CoreAddons
    KF5::I18n
    KF5::WidgetsAddons
)

install(TARGETS plasmanetworkmanagement_libreswanui  DESTINATION ${KDE_INSTALL_PLUGINDIR})

install(FILES plasmanetworkmanagement_libreswanui.desktop  DESTINATION ${KDE_INSTALL_KSERVICES5DIR})
