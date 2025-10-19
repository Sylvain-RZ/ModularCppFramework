# Documentation Development - Pour Mainteneurs

Cette documentation est destinée aux **mainteneurs et contributeurs du framework ModularCppFramework**.

## 🚀 Quick Start pour Contributeurs

**Nouveau contributeur?** Commencez par:
1. **[BUILD.md](BUILD.md)** - Compiler et tester le framework
2. **[IMPLEMENTATION.md](IMPLEMENTATION.md)** - Comprendre l'architecture interne
3. **[TEST_COVERAGE.md](TEST_COVERAGE.md)** - Écrire et exécuter les tests

## Guides de Développement

- **[BUILD.md](BUILD.md)** - Guide de build complet (compilation, options, debugging, CI/CD)
- **[CROSS_PLATFORM.md](CROSS_PLATFORM.md)** - Support multiplateforme (Linux, Windows, macOS)
- **[IMPLEMENTATION.md](IMPLEMENTATION.md)** - Détails techniques d'implémentation du framework
- **[TEST_COVERAGE.md](TEST_COVERAGE.md)** - Stratégie de tests et couverture (25 tests)
- **[TOOLS_TESTING.md](TOOLS_TESTING.md)** - Tests des générateurs et scripts (tools/, cmake/)
- **[PACKAGING.md](PACKAGING.md)** - Guide de packaging et distribution du SDK

## Pour Contribuer

Si vous souhaitez contribuer au framework:

1. **Build**: [BUILD.md](BUILD.md) - Compiler et tester le framework
2. **Comprendre**: [IMPLEMENTATION.md](IMPLEMENTATION.md) - Architecture interne du framework
3. **Tester**: [TEST_COVERAGE.md](TEST_COVERAGE.md) - Stratégie de tests et couverture
4. **Outils**: [TOOLS_TESTING.md](TOOLS_TESTING.md) - Tester les générateurs et scripts
5. **Distribuer**: [PACKAGING.md](PACKAGING.md) - Créer des releases et packages

## Structure du Projet

```
ModularCppFramework/
├── core/                   # Bibliothèque header-only (20 fichiers)
├── modules/                # Modules statiques (logger, networking, profiling, realtime)
├── plugins/                # Plugins d'exemple
├── tests/                  # Suite de tests (25 tests, 100% passent)
│   ├── unit/              # Tests unitaires (16 tests)
│   └── integration/       # Tests d'intégration (8 tests)
├── examples/               # Applications exemple (8 exemples)
├── tools/                  # Scripts de génération Python (create-plugin.py, create-application.py, package-application.py)
├── cmake/                  # Système CMake (générateurs, templates, packaging)
├── docs/
│   ├── sdk/               # Documentation pour utilisateurs
│   │   └── generators/    # Documentation générateurs
│   └── development/       # Documentation pour mainteneurs (vous êtes ici)
└── .github/workflows/     # CI/CD (multi-platform: Linux, Windows, macOS)
```

## Workflow de Contribution

1. **Fork** le repository
2. **Cloner** votre fork localement
3. **Compiler** avec les tests: `cmake -DBUILD_TESTS=ON .. && make -j$(nproc)`
4. **Tester**: `ctest -V` (100% des tests doivent passer)
5. **Développer** votre feature/bugfix
6. **Ajouter des tests** pour votre contribution
7. **Documenter** dans les fichiers appropriés
8. **Commit** avec messages clairs
9. **Push** vers votre fork
10. **Pull Request** vers le repository principal

Voir [CONTRIBUTING.md](../../CONTRIBUTING.md) pour plus de détails.

---

**Navigation:**
- ⬆️ [Documentation principale](../) - Retour à l'index docs/
- 📘 [Documentation SDK](../sdk/) - Pour utilisateurs du framework
- 🏠 [README principal](../../README.md) - Retour au README racine
