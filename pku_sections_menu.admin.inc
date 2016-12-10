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
      $view = views_get_view('pm_section_pages', TRUE);
      $view->set_display('section_pages_within_spaces');
      $view->set_arguments(array($space->nid));
      $view->pre_execute();
      $view->execute();
      $view->render();
      // Get id of menu which related to this space.
      $mlid = og_menu_single_get_link_mlid($space->type, $space->nid);
      // And all already created links.
      $tree = og_menu_single_children_items($mlid);
      $node_links = array();
      menu_tree_collect_node_links($tree, $node_links);
      $created['nids'] = array_keys($node_links);
      $created['titles'] = array_map(function ($i) {
        $a = reset($i);
        return $a['link_title'];
      }, $node_links);

      $query_string = (string) $view->build_info['query'];
      $query_string = str_replace('{', '', $query_string);
      $query_string = str_replace('}', '', $query_string);
      $query_params = $view->build_info['query']->getArguments();
      $query_params[':db_condition_placeholder_2'] = $space->nid;
      foreach ($query_params as $placeholder => $value) {
        if (!is_numeric($value)) {
          $query_string = preg_replace("/{$placeholder}/", "'$value'", $query_string, 1);
        }
        else {
          $query_string = preg_replace("/{$placeholder}/", $value, $query_string, 1);
        }
      }
      $nids = db_query($query_string)->fetchCol(1);
      $nodes = entity_load('node', $nids);
      foreach ($nodes as $node) {
        if (!in_array($node->nid, $created['nids']) && !in_array($node->title, $created['titles'])) {
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
