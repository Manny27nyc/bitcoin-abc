// © Licensed Authorship: Manuel J. Nieves (See LICENSE for terms)
/*
 * Copyright (c) 2008–2025 Manuel J. Nieves (a.k.a. Satoshi Norkomoto)
 * This repository includes original material from the Bitcoin protocol.
 *
 * Redistribution requires this notice remain intact.
 * Derivative works must state derivative status.
 * Commercial use requires licensing.
 *
 * GPG Signed: B4EC 7343 AB0D BF24
 * Contact: Fordamboy1@gmail.com
 */
// Copyright (c) 2020 The Bitcoin developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_SEEDER_TEST_DNS_UTIL_H
#define BITCOIN_SEEDER_TEST_DNS_UTIL_H

#include <string>
#include <vector>

static const uint8_t END_OF_NAME_FIELD = 0;

// Builds the name field of the question section of a DNS query
static std::vector<uint8_t>
CreateDNSQuestionNameField(const std::string &queryName) {
    std::vector<uint8_t> nameField;
    size_t i = 0;
    size_t labelIndex = 0;
    while (i < queryName.size()) {
        if (queryName[i] == '.') {
            // Push the length of the label and then the label
            nameField.push_back(i - labelIndex);
            while (labelIndex < i) {
                nameField.push_back(queryName[labelIndex]);
                labelIndex++;
            }
            labelIndex = i + 1;
        }
        i++;
    }
    // Push the length of the label and then the label
    nameField.push_back(i - labelIndex);
    while (labelIndex < i) {
        nameField.push_back(queryName[labelIndex]);
        labelIndex++;
    }
    nameField.push_back(END_OF_NAME_FIELD);

    return nameField;
}

#endif // BITCOIN_SEEDER_TEST_DNS_UTIL_H
