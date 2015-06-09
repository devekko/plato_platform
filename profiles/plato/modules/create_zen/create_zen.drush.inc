<?php

function create_zen_drush_command() {
  $items = array();
  $items['nirvana'] = array(
    'description' => "Use 'drush nirvana THEME_NAME' to download zen to this sites themes folder and generate subthemes with Poetic's default setup. This was developed for a multi site environment.",
    'arguments' => array(
      'theme_name' => 'Name for your theme.',
    ),
    'aliases' => array('nir'),
    'examples' => array(
      'drush nirvana abc' => 'Create a sub-theme for abc.com site and name the subtheme abc.',
    ),
    'bootstrap' => DRUSH_BOOTSTRAP_DRUPAL_SITE,
  );
  return $items;
}

function drush_create_zen_nirvana($theme_name) {
  // common variables
  $this_sites_theme_folder = conf_path() . "/themes";
  $module_path = drupal_get_path('module', 'create_zen');
  $subtheme_path = drush_normalize_path(drush_get_context('DRUSH_DRUPAL_ROOT') . '/' . $this_sites_theme_folder . '/' . $theme_name);
  $starter_path = drush_normalize_path(drush_get_context('DRUSH_DRUPAL_ROOT') . '/' . $this_sites_theme_folder . '/zen/STARTERKIT');
  $config_rb_file = $subtheme_path . '/config.rb';
  $init_scss_file = $subtheme_path . '/sass/_init.scss';
  $site_base_scss_file = $subtheme_path . '/sass/site/_base.scss';
  $global_base_scss_file = $subtheme_path . '/sass/global/_base.scss';
  $site_site_scss_file = $subtheme_path . '/sass/site/_site.scss';
  $sass_dir_path = $subtheme_path . '/sass/';

  // download zen
  drush_print("\nBeginning download of Zen theme.\n");
  exec('drush dl zen --destination=' . $this_sites_theme_folder . '', $output, $return);

  // copy the subtheme
  drush_op('drush_copy_dir', $starter_path, $subtheme_path);
  if (is_dir(dirname($theme_name))) {
    drush_print("\nSuccess creating " . $theme_name . " subtheme!\n");
  } else {
    drush_die(dt('Sorry, something went wrong and no subtheme was created.'));
  }

  // Rename the .info file.
  $subtheme_info_file = $subtheme_path . '/' . $theme_name . '.info';
  drush_op('rename', drush_normalize_path($subtheme_path . '/STARTERKIT.info.txt'), drush_normalize_path($subtheme_info_file));

  // Alter the contents of the .info file based on the command options.
  $alterations = array(
    '= Zen Sub-theme Starter Kit' => '= ' . $theme_name,
    ';scripts[] = js/script.js'   => 'scripts[] = js/script.js',
  );
  if ($description = drush_get_option('description')) {
    $alterations['Read the <a href="https://drupal.org/node/873778">online docs</a> or the included README.txt on how to create a theme with Zen.'] = $description;
  }
  drush_op('create_zen_file_str_replace', $subtheme_info_file, array_keys($alterations), $alterations);

  // Replace all occurrences of 'STARTERKIT' with the machine name of our sub theme.
  drush_op('create_zen_file_str_replace', $subtheme_path . '/theme-settings.php', 'STARTERKIT', $theme_name);
  drush_op('create_zen_file_str_replace', $subtheme_path . '/template.php', 'STARTERKIT', $theme_name);

  if (drush_confirm("Do you want Poetic's SMACCS subtheme setup?")) {
    // create poetic responsive scss architecture
    create_zen_poetic_scss($subtheme_path, $subtheme_info_file, $sass_dir_path);

    // create poetic sub theme base template overrides
    create_zen_poetic_template_overrides($this_sites_theme_folder, $subtheme_path, $module_path);

    // add explanations and imports to appropriate files
    create_zen_scss_explanations_and_import($global_base_scss_file, $site_site_scss_file);

    drush_print("\nPlease go read the SMACCS documentation here http://smacss.com/.\n");

    if (drush_confirm("Do you want Poetic's vertical rhythm SASS setup?")) {
      $base_font_size = drush_prompt('What is the base font size of your project?');
      $base_line_height = drush_prompt('What is the base line height of your project?');
      $init_alter = array(
        '$base-font-size:   16px;' => '$base-font-size:   ' . $base_font_size . 'px;',
        '$base-line-height: 24px;' => '$base-line-height: ' . $base_line_height . 'px;',
      );
      drush_op('create_zen_file_str_replace', $init_scss_file, array_keys($init_alter), $init_alter);
      // file_put_contents($subtheme_path . "/config.rb", "\n# this must be set in order for vertical rhythm to work in all browsers (namely chrome and IE)\nSass::Script::Number.precision = 8", FILE_APPEND);

      create_zen_vertical_rhythm_base_starter($site_base_scss_file);

      drush_print("\nPlease read http://compass-style.org/reference/compass/typography/vertical_rhythm/. Some sample code has been created in _base.scss to get you started with Htags.\n");
    }
  }

  // additional gem support
  if(drush_confirm("Do you want to add support for the breakpoint gem?")) {
    $gem_name = 'breakpoint';
    create_zen_gem_support($gem_name, $config_rb_file, $init_scss_file);
  }
}

function create_zen_file_str_replace($file_path, $find, $replace) {
  $file_path = drush_normalize_path($file_path);
  $file_contents = file_get_contents($file_path);
  $file_contents = str_replace($find, $replace, $file_contents);
  file_put_contents($file_path, $file_contents);
}

function create_zen_poetic_template_overrides($this_sites_theme_folder, $subtheme_path, $module_path) {
  $template_files = array(
    'page',
    'node',
    'html',
  );
  foreach ($template_files as $template) {
    copy($this_sites_theme_folder . '/zen/templates/' . $template . '.tpl.php', $subtheme_path . '/templates/' . $template . '.tpl.php');
    $current = file_get_contents($module_path . '/templates/' . $template . '.txt');
    file_put_contents($subtheme_path . '/templates/' . $template . '.tpl.php', $current);
  }
}

function create_zen_poetic_scss($subtheme_path, $subtheme_info_file, $sass_dir_path) {
  // create drupal poetic sass/breakpoint architecture
  mkdir($sass_dir_path . 'global', 0755);
  mkdir($sass_dir_path . 'site', 0755);

  // add global scss files
  $global_sass_files = array(
    '_base.scss',
    '_variables.scss',
    '_functions.scss',
    '_mixins.scss',
    '_extendables.scss',
  );
  foreach ($global_sass_files as $file_name) {
    fopen($sass_dir_path . "/global/" . $file_name,"w");
  }

  // add site smaccs files
  $site_sass_files = array(
    '_site.scss',
    '_base.scss',
    '_layout.scss',
    '_module.scss',
    '_state.scss',
    '_theme.scss',
  );
  foreach ($site_sass_files as $filename) {
    fopen($sass_dir_path . "/site/" . $filename,"w");
  }

  // add css imports to files
  $styles_scss_alter = array(
    '@import "print";' => "@import 'mixins';\n@import 'global/base';\n@import 'site/site';"
  );
  drush_op('create_zen_file_str_replace', $sass_dir_path . 'styles.scss', array_keys($styles_scss_alter), $styles_scss_alter);

}

function create_zen_gem_support($gem_name, $config_rb_file, $init_scss_file) {
  if(file_exists($config_rb_file) && file_exists($init_scss_file)) {
    $config_alter = array(
      "#require 'zen-grids'" => "#require 'zen-grids'\nrequire '" . $gem_name . "'",
    );
    $init_alter = array(
      '@import "mixins";' => "@import 'mixins';\n@import '" . $gem_name . "';"
    );
    drush_op('create_zen_file_str_replace', $config_rb_file, array_keys($config_alter), $config_alter);
    drush_op('create_zen_file_str_replace', $init_scss_file, array_keys($init_alter), $init_alter);
    drush_print("\nPlease don't forget to run gem install " . $gem_name . " locally!\n");
    if ($gem_name = 'breakpoint') {
      drush_print("\nSee http://breakpoint-sass.com/ for more documentation.\n");
    }
  } else {
    drush_set_error("Could not find config.rb or /sass/_init.scss in the subtheme, please check and make sure these files exist.");
  }
}

function create_zen_scss_explanations_and_import($global_base_scss_file, $site_site_scss_file) {
  // add @import to site and base scss files
  $global_base_scss_imports = "@import 'variables';\n@import 'functions';\n@import 'mixins';\n@import 'extendables';";
  file_put_contents($global_base_scss_file, $global_base_scss_imports);
  $site_site_scss_imports = "@import 'base';\n@import 'layout';\n@import 'module';\n@import 'state';\n@import 'theme';";
  file_put_contents($site_site_scss_file, $site_site_scss_imports);
}

function create_zen_vertical_rhythm_base_starter($site_base_scss_file) {
  // insert into site/base
$site_base_file_content = '@include establish-baseline;

h1 {
  @include adjust-font-size-to($h1-font-size);
  margin: 0 0 rhythm(1, $h1-font-size) 0; /* 1 line below */
}

h2 {
  @include adjust-font-size-to($h2-font-size);
  margin: rhythm(2, $h2-font-size) 0 rhythm(1, $h2-font-size) 0; /* 2 line above, 1 line below */
}

h3 {
  @include adjust-font-size-to($h3-font-size);
  margin: rhythm(2, $h3-font-size) 0 rhythm(1, $h3-font-size) 0; /* 2 line above, 1 line below */
}

h4 {
  @include adjust-font-size-to($h4-font-size);
  margin: rhythm(2, $h4-font-size) 0 rhythm(1, $h4-font-size) 0; /* 2 line above, 1 line below */
}';
file_put_contents($site_base_scss_file, $site_base_file_content);
}
