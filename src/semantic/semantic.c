#include "semantic.h"

void                    exit_block(ast_node_t **node) {

    g_printerr(SEMANTIC_ERROR_PLUGIN_O, (*node)->str);
    ast_free(node);
    exit(1);
}

static void             linked_free(void *content, size_t size) {
    (void)size;
    free(content);
}

void                    semantic_link(
                        GHashTable **symtable,
                        list_t **linked_elements,
                        list_t **linked_pads,
                        list_t **linked_pad_props) {

    link_element_all(*linked_elements);
    link_pad_all_always(symtable,*linked_pads);
    link_pad_all_request(symtable, *linked_pads);
    link_pad_all_sometimes(symtable, *linked_pads);
    link_pad_props(*linked_pad_props);

    /* Don't forget to free others lists */
    lstdel(linked_elements, &linked_free);
    //lstdel(&(linked->pads), &linked_free);

}

static void             semantic_pipeline(
                        ast_node_t **node,
                        config_pipeline_t *config_pipeline) {

    /* used by semantic_block_elements for gst_add_bin fn */
    linked_result_t     *linked = NULL;
    GstPipeline         *pipeline = NULL;
    ast_node_t          **elements = NULL;
    ast_node_t          *tmp = *node;
    list_t              *elements_dp = lstnew("elements", sizeof(char) * 9);

    semantic_block_pipelines(node);
    pipeline = (*node)->sdata->gstpipeline;

    (*node)->config_pipeline = config_pipeline;

    elements = aast_iblock_get(
            &((*node)->left),
            elements_dp);
    if (elements != NULL) {
        (*node)->symtable = g_hash_table_new (g_str_hash, g_str_equal);
        linked = semantic_block_elements(elements, &((*node)->symtable), pipeline);
        semantic_link(
                &((*node)->symtable),
                &(linked->elements),
                &(linked->pads),
                &(linked->pad_props));
    }
    *node = tmp;

}

supstream_t             *semantic_doc(ast_tree_t **root) {

    ast_node_t          *tmp = *root;
    ast_node_t          *priority_root = *root;
    ast_node_t          *tmp2 = *root;
    supstream_t         *supstream = (supstream_t *)malloc(sizeof(supstream_t));
    config_t            *config = config_new_default();
    config_pipeline_t   *config_pipeline = NULL;

    ast_node_t          *scalar_start_playing = NULL;
    ast_node_t          *scalar_start_paused = NULL;
    ast_node_t          *scalar_start_ready = NULL;
    ast_node_t          *scalar_start_null = NULL;
    semantic_data_t     *sdata = NULL;

    *root = (*root)->left;
    priority_root = *root;

    /* Priority blocks (global options) */

    while (priority_root) {
        if (AST_IBLOCK_IS (priority_root, "supstream") && priority_root->left) {
            semantic_config(*root, &config);
        }
        priority_root = priority_root->right;
    }

    /* Apply each pipelines */

    while (*root) {

        tmp2 = *root;
        if (AST_IBLOCK_IS (*root, "elements") && (*root)->left) {
            *root = (*root)->left;
        } else if (AST_IBLOCK_IS (*root, "pipelines") && (*root)->left) {
            *root = (*root)->left;

            while (*root != NULL) {

                if (AST_IS_IBLOCK (*root) == TRUE) {
                    
                    

                    /* Specific pipeline config */

                    config_pipeline = config_new_default_pipeline();
                    semantic_config_pipeline(*root, &config_pipeline);
                    semantic_pipeline(root, config_pipeline);

                    /* Set delay */

                    semantic_config_set_delay(config_pipeline, (*root)->sdata->gstpipeline);

                    /* State scheduler */

                    scalar_start_playing = ast_iscalar_get_by_key(*root, "playing_state");
                    if (scalar_start_playing != NULL) {
                        sdata = (semantic_data_t *)(*root)->sdata;
                        semantic_line_start_playing(GST_ELEMENT (sdata->gstpipeline), scalar_start_playing->right->str);
                    }
                    scalar_start_paused = ast_iscalar_get_by_key(*root, "paused_state");
                    if (scalar_start_paused != NULL) {
                        sdata = (semantic_data_t *)(*root)->sdata;
                        semantic_line_start_paused(GST_ELEMENT (sdata->gstpipeline), scalar_start_playing->right->str);
                    }
                    scalar_start_ready = ast_iscalar_get_by_key(*root, "ready_state");
                    if (scalar_start_ready != NULL) {
                        sdata = (semantic_data_t *)(*root)->sdata;
                        semantic_line_start_ready(GST_ELEMENT (sdata->gstpipeline), scalar_start_playing->right->str);
                    }
                    scalar_start_null = ast_iscalar_get_by_key(*root, "null_state");
                    if (scalar_start_null != NULL) {
                        sdata = (semantic_data_t *)(*root)->sdata;
                        semantic_line_start_null(GST_ELEMENT (sdata->gstpipeline), scalar_start_playing->right->str);
                    }

                }

                *root = (*root)->right;
            }
        }
        *root = tmp2;
        *root = (*root)->right;

    }

    /* End ptr */

    *root = tmp;
    supstream->root = root;
    supstream->config = config;
    return (supstream);

}
