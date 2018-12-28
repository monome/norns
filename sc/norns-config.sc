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
    Norns.addIncludePath("/home/we/norns/sc/core");
    Norns.addIncludePath("/home/we/norns/sc/engines");
    Norns.addIncludePath("/home/we/norns/sc/ugens");
    Norns.addIncludePath("/home/we/dust");
  }
}
