package com.valhalla.valhalla

internal class ValhallaKotlin {
  companion object {
    init {
      System.loadLibrary("valhalla-wrapper")
    }
  }

  external fun route(request: String, configPath: String): String

  external fun optimizedRoute(request: String, configPath: String): String

  external fun height(request: String, configPath: String): String

  external fun locate(request: String, configPath: String): String
}
