<?php

/**
 * @file
 * Represents configuration forms and all logic related to them.
 */

/**
 * Form builder for the generate links process.
 */
function pku_sections_menu_settings_form($form, $form_state) {
  $form['save'] = array(
    '#type' => 'submit',
    '#value' => t('Generate'),
    '#suffix' => t('This will create all missing menu links for each space.'),
  );
  return $form;
}

/**
 * Custom submit handler for the generate links process.
 *
 * @see pku_sections_menu_settings_form()
 */
function pku_sections_menu_settings_form_submit($form, $form_state) {
  // Get all oa_space Ct nodes.
  $query = new EntityFieldQuery();
  $query->entityCondition('entity_type', 'node')
    ->entityCondition('bundle', 'oa_space')
    ->propertyCondition('status', NODE_PUBLISHED);
  $result = $query->execute();
  if (isset($result['node'])) {
    $nids = array_keys($result['node']);
    $spaces = entity_load('node', $nids);
    $links = array();
    foreach ($spaces as $space) {
      // Get all related sections.
      $view = views_get_view('pm_section_pages');
      $view->set_display('section_pages_within_spaces');
      $view->set_arguments(array(1 => $space->nid));
      $view->pre_execute();
      $view->execute();
      // Get id of menu which related to this space.
      $mlid = og_menu_single_get_link_mlid($space->type, $space->nid);
      // And all already created links.
      $tree = og_menu_single_children_items($mlid);
      $node_links = array();
      menu_tree_collect_node_links($tree, $node_links);
      $created = array_keys($node_links);
      $nids = array_map(function ($i) {
        return $i->nid;
      }, $view->result);
      $nodes = entity_load('node', $nids);
      foreach ($nodes as $node) {
        if (!in_array($node->nid, $created)) {
          $links[] = array(
            'plid' => $mlid,
            'link_title' => $node->title,
            'link_path' => 'node/' . $node->nid,
            'menu_name' => OG_MENU_SINGLE_MENU_NAME,
            'options' => array(
              'attributes' => array(
                'title' => t('Menu for group') . ' ' . $node->title,
              ),
            ),
          );
        }
      }
    }

    $batch = array(
      'operations' => array(array('_pku_sections_menu_batch', array($links))),
      'finished' => '_pku_sections_menu_batch_finished',
      'title' => t('Generating menu links..'),
      'init_message' => t('Generating initializes...'),
      'progress_message' => t('Generating links.'),
      'error_message' => t('Generating failed. Check reports.'),
    );
    batch_set($batch);
  }
}
