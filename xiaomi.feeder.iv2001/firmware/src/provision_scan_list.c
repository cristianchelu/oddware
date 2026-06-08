/*
 * Provisioning Wi-Fi scan list helpers — spec/30-processes/provisioning-flow.md
 */

#include <string.h>

#include "provision_scan_list.h"

void provision_scan_list_clear(provision_scan_list_t *list)
{
    if (list == NULL) {
        return;
    }

    memset(list, 0, sizeof(*list));
}

static int scan_ap_index(const provision_scan_list_t *list, const char *ssid)
{
    size_t i;

    if (list == NULL || ssid == NULL) {
        return -1;
    }

    for (i = 0; i < list->count; i++) {
        if (strcmp(list->aps[i].ssid, ssid) == 0) {
            return (int)i;
        }
    }

    return -1;
}

static void scan_insert_sorted(provision_scan_list_t *list, const provision_scan_ap_t *ap)
{
    size_t i;
    size_t insert_at = list->count;

    if (list == NULL || ap == NULL) {
        return;
    }

    if (list->count >= PROVISION_SCAN_MAX_APS) {
        if (ap->rssi <= list->aps[list->count - 1].rssi) {
            return;
        }

        list->count--;
    }

    for (i = 0; i < list->count; i++) {
        if (ap->rssi > list->aps[i].rssi) {
            insert_at = i;
            break;
        }
    }

    list->count++;

    for (i = list->count - 1; i > insert_at; i--) {
        list->aps[i] = list->aps[i - 1];
    }

    list->aps[insert_at] = *ap;
}

size_t provision_scan_list_merge(provision_scan_list_t *list,
                                 const provision_scan_ap_t *raw,
                                 size_t raw_count,
                                 const char *exclude_ssid)
{
    size_t i;

    if (list == NULL || raw == NULL) {
        return 0;
    }

    for (i = 0; i < raw_count; i++) {
        const provision_scan_ap_t *entry = &raw[i];
        int existing;
        provision_scan_ap_t merged;

        if (entry->ssid[0] == '\0') {
            continue;
        }

        if (exclude_ssid != NULL && exclude_ssid[0] != '\0'
            && strcmp(entry->ssid, exclude_ssid) == 0) {
            continue;
        }

        existing = scan_ap_index(list, entry->ssid);
        if (existing >= 0) {
            if (entry->rssi <= list->aps[existing].rssi) {
                continue;
            }

            list->aps[existing].rssi = entry->rssi;
            merged = list->aps[existing];
            memmove(&list->aps[existing],
                    &list->aps[existing + 1],
                    (list->count - (size_t)existing - 1) * sizeof(list->aps[0]));
            list->count--;
            scan_insert_sorted(list, &merged);
            continue;
        }

        scan_insert_sorted(list, entry);
    }

    return list->count;
}
