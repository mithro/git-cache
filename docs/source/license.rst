License
=======

git-cache is licensed under the same terms as the Git project itself.

GPL Version 2 License
----------------------

.. code-block:: text

   git-cache - A high-performance Git repository caching tool
   Copyright (C) 2025 git-cache contributors

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License along
   with this program; if not, write to the Free Software Foundation, Inc.,
   51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.

License Rationale
-----------------

Why GPL v2?
^^^^^^^^^^^

git-cache is licensed under GPL v2 for the following reasons:

1. **Consistency with Git**: As a tool that extends Git functionality, using the same license ensures compatibility and consistency.

2. **Copyleft Protection**: GPL v2 ensures that improvements and modifications to git-cache remain open source and available to the community.

3. **Community Contribution**: The license encourages contributions back to the project while protecting the open source nature of the codebase.

4. **Legal Clarity**: GPL v2 is a well-established license with clear terms and extensive legal precedent.

License Compatibility
---------------------

Compatible Licenses
^^^^^^^^^^^^^^^^^^^

Code licensed under the following licenses can be incorporated into git-cache:

* **GPL v2 or later** - Direct compatibility
* **GPL v3** - Compatible (as GPL v2+ allows v3)
* **LGPL v2.1/v3** - Compatible for linking
* **BSD licenses** - Compatible (can be relicensed under GPL)
* **MIT License** - Compatible (can be relicensed under GPL)
* **Apache 2.0** - Generally compatible with proper attribution

Incompatible Licenses
^^^^^^^^^^^^^^^^^^^^^^

The following licenses are generally incompatible with GPL v2:

* **Proprietary/Commercial** licenses
* **GPL v3-only** code (without "or later" clause)
* **AGPL v3** (different copyleft terms)
* Licenses with GPL-incompatible restrictions

Third-Party Components
----------------------

Dependencies
^^^^^^^^^^^^

git-cache uses the following third-party libraries:

**libcurl**
* License: MIT/X derivative license
* Compatibility: Compatible with GPL v2
* Usage: HTTP/HTTPS operations for GitHub API

**libjson-c**
* License: MIT License
* Compatibility: Compatible with GPL v2
* Usage: JSON parsing for GitHub API responses

**System Libraries**
* Various system libraries (libc, etc.)
* Generally compatible or exempted under GPL system library clause

Development Dependencies
^^^^^^^^^^^^^^^^^^^^^^^^

Additional tools used in development (not distributed):

**Sphinx** (Documentation)
* License: BSD License
* Usage: Documentation generation

**Doxygen** (API Documentation)
* License: GPL v2
* Usage: API documentation generation

**Test Frameworks**
* Various testing utilities under compatible licenses

Attribution Requirements
------------------------

When Redistributing
^^^^^^^^^^^^^^^^^^^

When redistributing git-cache or derivative works, you must:

1. **Include License Text**: Provide the full GPL v2 license text
2. **Preserve Copyright**: Keep all copyright notices intact
3. **Provide Source**: Make source code available (for binary distributions)
4. **Document Changes**: Clearly mark any modifications made

Example Attribution
^^^^^^^^^^^^^^^^^^^

.. code-block:: text

   This software includes git-cache, which is licensed under the
   GNU General Public License version 2.
   
   git-cache Copyright (C) 2025 git-cache contributors
   
   For the complete license text, see LICENSE file or visit:
   https://www.gnu.org/licenses/old-licenses/gpl-2.0.html

Commercial Use
--------------

Commercial Usage Rights
^^^^^^^^^^^^^^^^^^^^^^^

GPL v2 explicitly allows commercial use with these requirements:

* **Source Availability**: Must provide source code to recipients
* **License Propagation**: Must license derivative works under GPL v2
* **No Additional Restrictions**: Cannot add additional use restrictions
* **Patent Grant**: Implicit patent license for GPL-covered code

**Examples of Allowed Commercial Use:**
* Using git-cache in commercial development workflows
* Packaging git-cache with commercial products (with source)
* Providing git-cache as part of commercial services
* Creating commercial training/consulting around git-cache

**Examples of Restricted Use:**
* Creating proprietary derivatives without source release
* Adding additional licensing restrictions
* Patent-based restrictions on GPL-covered functionality

Dual Licensing
^^^^^^^^^^^^^^

Currently, git-cache is only available under GPL v2. Dual licensing under commercial terms is not offered at this time.

For commercial licensing inquiries, contact the project maintainers.

Contributing and Copyright
--------------------------

Contributor License Agreement
^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Contributors to git-cache agree to:

1. **License Compatibility**: Only contribute code compatible with GPL v2
2. **Copyright Assignment**: Grant project right to use contributions under GPL v2
3. **Original Work**: Ensure contributions are original or properly licensed
4. **No Conflicting Obligations**: Have right to make the contribution

**By submitting a pull request, contributors agree that:**
* Their contribution is original work or properly licensed
* They have the right to contribute the code under GPL v2
* The contribution can be distributed under GPL v2 terms

Copyright Ownership
^^^^^^^^^^^^^^^^^^^

* **Project Copyright**: Held collectively by "git-cache contributors"
* **Individual Contributions**: Contributors retain copyright to their specific contributions
* **Collective Work**: The overall work is owned by the contributor community
* **No Copyright Assignment**: Contributors do not assign away their copyright

Legal Notices
-------------

Disclaimer
^^^^^^^^^^

.. code-block:: text

   This software is provided "AS IS" without warranty of any kind.
   See the GNU General Public License for complete warranty disclaimers.

Trademark Notice
^^^^^^^^^^^^^^^^

* "git-cache" is a descriptive name, not a registered trademark
* "Git" is a trademark of Software Freedom Conservancy
* "GitHub" is a trademark of GitHub, Inc.

Patent Notice
^^^^^^^^^^^^^

GPL v2 includes an implicit patent license for GPL-covered code. Contributors grant a patent license for their contributions under GPL v2 terms.

International Considerations
----------------------------

Export Control
^^^^^^^^^^^^^^

git-cache may be subject to export control laws in some jurisdictions. Users are responsible for compliance with applicable export control regulations.

Jurisdiction
^^^^^^^^^^^^

The GPL v2 license is interpreted under the laws where the copyright holder resides. For git-cache, this follows the jurisdiction of the primary maintainers.

Getting Help with Licensing
----------------------------

Questions about License
^^^^^^^^^^^^^^^^^^^^^^^

For questions about licensing:

1. **Read the License**: Full GPL v2 text is available at https://www.gnu.org/licenses/old-licenses/gpl-2.0.html
2. **Check FAQ**: GPL FAQ at https://www.gnu.org/licenses/gpl-faq.html
3. **Contact Maintainers**: For project-specific questions
4. **Legal Advice**: Consult qualified legal counsel for legal advice

Common Questions
^^^^^^^^^^^^^^^^

**Q: Can I use git-cache in my commercial project?**
A: Yes, GPL v2 allows commercial use. You must provide source code to recipients and license derivative works under GPL v2.

**Q: Can I create a proprietary tool based on git-cache?**
A: No, derivative works must be licensed under GPL v2 and source code must be available.

**Q: Can I bundle git-cache with my proprietary software?**
A: Yes, as separate programs. If you create a derivative work, it must be GPL v2.

**Q: Do I need to release my configuration files?**
A: No, configuration files and data files are generally not considered derivative works.

License History
---------------

* **v1.0.0**: Initial release under GPL v2
* **Future**: License changes require community consensus

For the complete, legally binding license text, see the LICENSE file in the source repository or visit https://www.gnu.org/licenses/old-licenses/gpl-2.0.html.