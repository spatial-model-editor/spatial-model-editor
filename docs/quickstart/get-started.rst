Setting the stage
=================

How to read this documentation
-------------------------------
Welcome to the spatial-model-editor (SME)!
This documentation is divided into several parts. Of those, the Getting started section, which begins with this page, is the most important one that every new user should work through. It will teach you how to create a new model in SME, and you will become familiar with all important functions the software has to offer along the way.

Secondly, there is a library of example models ready to run under :doc:`Example models <../examples/example-overview>`. Once you are done with the Getting started section, you can look through those to get some experience with using SME. They have been selected to roughly cover all the different aspects of the software.

The third part, :doc:`User guides <../userguides/work-with-copasi>`, is a collection of more specialized articles that treat specific use cases. Have a look at them after you read the Getting started section.

The :doc:`Python interface <../sme/sme>` section describes how to use SME from Python, for example to run simulations from a script or a Jupyter notebook.

The :doc:`Reference documentation <../reference/dune>` section contains more detailed information about the inner workings of SME. Treat this as a library in which you can look up specific information whenever you need it.

Finally, the :doc:`Developer documentation <../developer/core>` section is only relevant if you intend to contribute to SME's source code. It contains information about the code structure and how to run and add tests.

Installation
------------

No installation required, just download and run the executable for your operating system:

* |icon-linux-x64|_ `Linux x64 <https://github.com/spatial-model-editor/spatial-model-editor/releases/latest/download/spatial-model-editor-X64>`_
* |icon-linux-arm64|_ `Linux Arm64 <https://github.com/spatial-model-editor/spatial-model-editor/releases/latest/download/spatial-model-editor-ARM64>`_
* |icon-windows-x64|_ `Windows x64 <https://github.com/spatial-model-editor/spatial-model-editor/releases/latest/download/spatial-model-editor.exe>`_
* |icon-windows-arm64|_ `Windows Arm64 <https://github.com/spatial-model-editor/spatial-model-editor/releases/latest/download/spatial-model-editor-ARM64.exe>`_
* |icon-macos-arm64|_ `macOS Arm64 <https://github.com/spatial-model-editor/spatial-model-editor/releases/latest/download/spatial-model-editor-ARM64.dmg>`_

.. tip::
   You may have to give permission before your operating system will run the executable:

   * Windows: "More info"->"Run anyway"
   * MacOS: right-click open
   * Linux: ``chmod +x spatial-model-editor-X64`` (or ``spatial-model-editor-ARM64``)

.. note::
   On linux some additional system libraries are required that may not be installed by default. To install them:

   *  Fedora/RHEL/CentOS: ``sudo yum install xcb-util-image xcb-util-keysyms xcb-util-renderutil xcb-util-wm``

.. |icon-linux-x64| image:: ../img/icon-linux.png
.. _icon-linux-x64: https://github.com/spatial-model-editor/spatial-model-editor/releases/latest/download/spatial-model-editor-X64

.. |icon-linux-arm64| image:: ../img/icon-linux.png
.. _icon-linux-arm64: https://github.com/spatial-model-editor/spatial-model-editor/releases/latest/download/spatial-model-editor-ARM64

.. |icon-windows-x64| image:: ../img/icon-windows.png
.. _icon-windows-x64: https://github.com/spatial-model-editor/spatial-model-editor/releases/latest/download/spatial-model-editor.exe

.. |icon-windows-arm64| image:: ../img/icon-windows.png
.. _icon-windows-arm64: https://github.com/spatial-model-editor/spatial-model-editor/releases/latest/download/spatial-model-editor-ARM64.exe

.. |icon-macos-arm64| image:: ../img/icon-osx.png
.. _icon-macos-arm64: https://github.com/spatial-model-editor/spatial-model-editor/releases/latest/download/spatial-model-editor-ARM64.dmg
