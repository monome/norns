Norns {
  *addIncludePath { arg path;
    if(LanguageConfig.includePaths.includesEqual(path).not, {
        path.debug("Adding path");
        LanguageConfig.addIncludePath(path);
        LanguageConfig.store(LanguageConfig.currentPath);
        ^true
        });
    ^false
  }

  *initClass {
    Norns.addIncludePath(Platform.userHomeDir ++ "/norns/sc/core");
    Norns.addIncludePath(Platform.userHomeDir ++ "/norns/sc/engines");
    Norns.addIncludePath(Platform.userHomeDir ++ "/norns/sc/ugens");
    Norns.addIncludePath(Platform.userHomeDir ++ "/dust");
  }
}
