dic = {
    # Arena Battles: own app id and name, so it installs next to the official VCMI apps
    "applicationIdSuffix": ".arenabattles",
    "applicationLabel": "Arena Battles",
    "applicationVariant": "daily",
    "signingConfig": "dailySigning",
}
print(";".join([f"{key}={value}" for key, value in dic.items()]))
