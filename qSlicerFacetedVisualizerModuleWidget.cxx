/*==============================================================================

  Program: 3D Slicer

  Portions (c) Copyright Brigham and Women's Hospital (BWH) All Rights Reserved.

  See COPYRIGHT.txt
  or http://www.slicer.org/copyright/copyright.txt for details.

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.

==============================================================================*/

// TODO: Add methods for loading the sql database and processing queries
// TODO: Add methods for accessing vtkMRMLModelHierarchy code and updating MRMLnode

// Qt includes
#include <QtGui>
#include <QTreeView>
#include <QStandardItemModel>
#include <QPalette>
//#include <QItemSelectionModel>

// SlicerQt includes
#include "qSlicerFacetedVisualizerModuleWidget.h"
#include "ui_qSlicerFacetedVisualizerModule.h"

// logic includes
#include "vtkSlicerFacetedVisualizerLogic.h"


#include <vector>
#include <string>

//-----------------------------------------------------------------------------
/// \ingroup Slicer_QtModules_FacetedVisualizer
class qSlicerFacetedVisualizerModuleWidgetPrivate: public Ui_qSlicerFacetedVisualizerModule
{
	Q_DECLARE_PUBLIC(qSlicerFacetedVisualizerModuleWidget);
protected:
	qSlicerFacetedVisualizerModuleWidget* const q_ptr;
public:
  qSlicerFacetedVisualizerModuleWidgetPrivate(qSlicerFacetedVisualizerModuleWidget& object);
  ~qSlicerFacetedVisualizerModuleWidgetPrivate();
  
  vtkSlicerFacetedVisualizerLogic* logic() const;
};

//-----------------------------------------------------------------------------
// qSlicerFacetedVisualizerModuleWidgetPrivate methods

//-----------------------------------------------------------------------------
qSlicerFacetedVisualizerModuleWidgetPrivate::qSlicerFacetedVisualizerModuleWidgetPrivate(qSlicerFacetedVisualizerModuleWidget& object) : q_ptr(&object)
{
}

qSlicerFacetedVisualizerModuleWidgetPrivate::~qSlicerFacetedVisualizerModuleWidgetPrivate()
{

}

vtkSlicerFacetedVisualizerLogic* qSlicerFacetedVisualizerModuleWidgetPrivate::logic() const
{
	Q_Q(const qSlicerFacetedVisualizerModuleWidget);
	return vtkSlicerFacetedVisualizerLogic::SafeDownCast(q->logic());
}
//-----------------------------------------------------------------------------
// qSlicerFacetedVisualizerModuleWidget methods

//-----------------------------------------------------------------------------
qSlicerFacetedVisualizerModuleWidget::qSlicerFacetedVisualizerModuleWidget(QWidget* _parent)
  : Superclass( _parent )
  , d_ptr( new qSlicerFacetedVisualizerModuleWidgetPrivate(*this))
{
}

//-----------------------------------------------------------------------------
qSlicerFacetedVisualizerModuleWidget::~qSlicerFacetedVisualizerModuleWidget()
{
}

//-----------------------------------------------------------------------------
void qSlicerFacetedVisualizerModuleWidget::setup()
{
  Q_D(qSlicerFacetedVisualizerModuleWidget);
  d->setupUi(this);

  connect( d->pushButton_browse, SIGNAL(clicked()), this, SLOT( onOntologyFileName()));
  connect( d->lineEdit_query, SIGNAL(textChanged(const QString &)), 
           this, SLOT(onQueryTextChanged( const QString &)));
  connect( d->pushButton_query, SIGNAL(clicked()), this, SLOT( onQuery()));

   maxQueryLog = 20;
   setDBFile = false;

   connect(d->pushButton_mrmlDB, SIGNAL(clicked()), this, SLOT( onMatchDBMRMLAtom()));
   connect(d->lineEdit_mrml, SIGNAL(textChanged(const QString&)),
		   this, SLOT( onMRMLAtomChanged(const QString &)));
   connect(d->pushButton_favorites, SIGNAL(clicked()), this, SLOT(onCheckedFavorites()));

	favoritesModel = new QStandardItemModel;


   this->Superclass::setup();
  
}


void qSlicerFacetedVisualizerModuleWidget::onCheckedFavorites()
{
	Q_D(qSlicerFacetedVisualizerModuleWidget);

	vtkSlicerFacetedVisualizerLogic *logic = d->logic();
	// append the favorite to the favorites tree
	std::string q = logic->GetQuery();
	this->appendToFavorites(q);
}


//-----------------------------------------------------------------------------
void qSlicerFacetedVisualizerModuleWidget::onOntologyFileName()
{
   Q_D(qSlicerFacetedVisualizerModuleWidget);
   
   QString path;
   path = QFileDialog::getOpenFileName(this, "Choose a file to open (extension .sqlite3)", 
                                       QString::null, QString::null);
   d->lineEdit->setText(path);

   this->setDBFile = true;
   // sync the mrml models with the ontology file
   vtkSlicerFacetedVisualizerLogic *logic = d->logic();
   logic->SetDBFileName(path.toStdString());
   std::cout<<" Set the Database file name .. Now synchronizing atlas with the DB... "<<std::endl;

   logic->SynchronizeAtlasWithDB(this->matchingDBAtoms, this->unMatchedMRMLAtoms);

   // add the results of the mrmlElements to the mrmlTree
   this->updateAtlasNodesTree();

   this->readyNextMRMLDBAtomMatch = true;
   int count = 0;

   if(unMatchedMRMLAtoms.size() > 0)
   {
	  if(this->readyNextMRMLDBAtomMatch)
	  {
		 this->readyNextMRMLDBAtomMatch = false;
		 this->selectedMRMLAtomIndex = count;

		 //this->GetUserMatchesForMRMLTerms(unMatchedMRMLAtoms[count]);
	  }
   }
   std::cout<<" Done syncrhonizing atlas with DB... "<<std::endl;

}


//-----------------------------------------------------------------------------
void qSlicerFacetedVisualizerModuleWidget::onQueryTextChanged(const QString &text)
{
	Q_D(qSlicerFacetedVisualizerModuleWidget);
	vtkSlicerFacetedVisualizerLogic *logic = d->logic();

    logic->SetQuery(text.toStdString());
}

//-----------------------------------------------------------------------------
void qSlicerFacetedVisualizerModuleWidget::onQuery()
{
  // process the query
	Q_D(qSlicerFacetedVisualizerModuleWidget);

	vtkSlicerFacetedVisualizerLogic *logic = d->logic();

	// todo: need a QDialog here that tells the user that they need to set the DB file
	if(!this->setDBFile)
	{
		QString warningText = "< font color = 'red'>Database file not set</font>";
		d->label_warning->setText(warningText);
		QPalette pal;
		pal.setColor(QPalette::Text, Qt::red);
		d->label_warning->setPalette(pal);
	}
	else
	{
		QString warningText = "";
		d->label_warning->setText(warningText);
		QPalette pal;
		pal.setColor(QPalette::Text, Qt::red);
		d->label_warning->setPalette(pal);
	}
	bool visualizedResults = logic->ProcessQuery();

	// display the results of query on treeview and comment box
	this->UpdateResultsTree(visualizedResults);
	if(queryLog.size() < maxQueryLog)
	{
		queryLog.push_back(logic->GetQuery());
	}
	else
	{
		queryLog.pop_front();
		queryLog.push_back(logic->GetQuery());
	}

	// update the queryLog
	this->UpdateQueryLogView();
}

//-----------------------------------------------------------------------------
// this is triggered when the user selected an item from the query log. then that item is automatically put
// for search and display

void qSlicerFacetedVisualizerModuleWidget::onQueryLogItemSelected(const QItemSelection &,
		const QItemSelection &)
{

	Q_D(qSlicerFacetedVisualizerModuleWidget);
	QList< QModelIndex> selectedIndices = d->treeViewQueryLog->selectionModel()->selectedIndexes();
	QString selectedText = "";
	for (int i = 0; i < selectedIndices.size(); ++i)
	{
		const QModelIndex index = selectedIndices[i];
		QString text = index.data(Qt::DisplayRole).toString();
		selectedText += text;
		if(i < selectedIndices.size()-1)
		{
			selectedText += "+";
		}
	}

	d->lineEdit_query->setText(selectedText);
}


void qSlicerFacetedVisualizerModuleWidget::onFavoritesItemSelected(const QItemSelection &,
		const QItemSelection &)
{

	Q_D(qSlicerFacetedVisualizerModuleWidget);
	//const QModelIndex index = d->treeViewFavorites->selectionModel()->currentIndex();
	QList< QModelIndex> selectedIndices = d->treeViewFavorites->selectionModel()->selectedIndexes();
	QString selectedText = "";

	for (int i = 0; i < selectedIndices.size(); ++i)
	{
	  const QModelIndex index = selectedIndices[i];//d->treeViewResults->selectionModel()->currentIndex();

	  QString text = index.data(Qt::DisplayRole).toString();
	  selectedText += text;

	  if(i < selectedIndices.size()-1)
	  {
		  selectedText += "+";
	  }
	}
	d->lineEdit_query->setText(selectedText);
}



//-----------------------------------------------------------------------------
// this is triggered when the user selects an item from the results display tree. This helps to do drill-down
// searches
void qSlicerFacetedVisualizerModuleWidget::onTreeItemSelected(const QItemSelection &/*oldItem*/,
		const QItemSelection & /*newItem*/)
{
	std::cout<<" tree item selected.."<<std::endl;


	Q_D(qSlicerFacetedVisualizerModuleWidget);

	QList< QModelIndex> selectedIndices = d->treeViewResults->selectionModel()->selectedIndexes();
	QString text = "";

	for (int i = 0; i < selectedIndices.size(); ++i)
	{
	  const QModelIndex index = selectedIndices[i];//d->treeViewResults->selectionModel()->currentIndex();

	  QString selectedText = index.data(Qt::DisplayRole).toString();

	  // we need to get the selection's parent to update the query correctly
	  // change the query edit box
	  QModelIndex seekroot = index.parent();
	  QModelIndex currParent = index.parent();
	  while(seekroot.parent() != QModelIndex())
	  {
		currParent = seekroot;
		seekroot = seekroot.parent();
	  }
	  QString parentText = currParent.data(Qt::DisplayRole).toString();
	  std::cout<<"Selected text "<<selectedText.toStdString()<<" parent node "<<parentText.toStdString()<<std::endl;
	  std::string ptext = parentText.toStdString();
	  size_t pos = ptext.find("-");
	  if(pos == std::string::npos)
	  {
	 	text += parentText+";"+selectedText;
	  }
	  else
	  {
		  text += selectedText;
	  }
	  if(i < selectedIndices.size()-1)
	  {
		  text += "+";
	  }
	}
	d->lineEdit_query->setText(text);
}

//-----------------------------------------------------------------------------
void qSlicerFacetedVisualizerModuleWidget::UpdateResultsTree(bool visualizedResults)
{

	Q_D(qSlicerFacetedVisualizerModuleWidget);
	vtkSlicerFacetedVisualizerLogic *logic = d->logic();

	std::vector< std::string > queries;
	std::vector< std::vector< std::string > > queryResults;

	logic->GetQueryResults(queryResults, queries);

	QStandardItemModel *standardModel = new QStandardItemModel;
	QStandardItem *rootNode = standardModel->invisibleRootItem();


	QString text=QString::fromStdString("");
	for (unsigned n = 0; n < queries.size(); ++n)
	{
		QStandardItem *qItem = new QStandardItem(QString::fromStdString(queries[n]));
		rootNode->appendRow(qItem);
		std::vector< std::string > comments;
		for (unsigned nr = 0; nr < queryResults[n].size(); nr++)
		{
			size_t pos = queryResults[n][nr].find(";");
			if(pos != std::string::npos)
			{
				std::string pred = queryResults[n][nr].substr(0, pos);
				if(visualizedResults)
				{
					size_t p1 = pred.find("comment");
					if(p1 != std::string::npos)
					{
						p1 = queryResults[n][nr].substr(pos).find(";");

						std::string str = (queryResults[n][nr].substr(pos)).substr(p1+1)+"\n";
						comments.push_back(str);
					}
				}
				else
				{
					std::string str = queryResults[n][nr].substr(pos+1)+"  \n";
					comments.push_back(str);
				}
			}
			else
			{
			  QStandardItem *resItem = new QStandardItem(QString::fromStdString(queryResults[n][nr]));
			  if(!visualizedResults)
			  {
				 comments.push_back(queryResults[n][nr]);
			  }
			  //else
			  //{
			    qItem->appendRow(resItem);
			  //}
			}
		}
		std::string str = "\n"+queries[n]+"\n--";
		text += QString::fromStdString(str);
		if(comments.size() > 0)
		{
			for (unsigned nc = 0; nc < comments.size(); ++nc)
			{
				text += QString::fromStdString(comments[nc])+"\n";
			}
		}
	}
	d->plainTextEditCommentBox->setPlainText(text);
	d->treeViewResults->setModel(standardModel);
	d->treeViewResults->expandAll();

	QItemSelectionModel *selectionModel = d->treeViewResults->selectionModel();
	connect (selectionModel, SIGNAL(selectionChanged(const QItemSelection &,
			  const QItemSelection &)), this,
			  SLOT(onTreeItemSelected(const QItemSelection &, const QItemSelection &)));

}


void qSlicerFacetedVisualizerModuleWidget::appendToFavorites(std::string& text)
{
   Q_D(qSlicerFacetedVisualizerModuleWidget);

   bool found = false;
   for (unsigned i = 0; !found && i < this->favoriteQueries.size(); ++i)
   {
	   found = this->favoriteQueries[i] == text;
   }
   if(!found)
   {
	   this->favoriteQueries.push_back(text);
	   QStandardItem *favoritesRootNode = this->favoritesModel->invisibleRootItem();

	   QStandardItem *qItem = new QStandardItem(QString::fromStdString(text));
	   favoritesRootNode->appendRow(qItem);
	   d->treeViewFavorites->setModel(favoritesModel);
	   d->treeViewFavorites->expandAll();

	   QItemSelectionModel *selectionModel = d->treeViewFavorites->selectionModel();
	   connect (selectionModel, SIGNAL(selectionChanged(const QItemSelection &,
	   				  const QItemSelection &)), this,
	   				  SLOT(onFavoritesItemSelected(const QItemSelection &, const QItemSelection &)));
   }

}

void qSlicerFacetedVisualizerModuleWidget::UpdateQueryLogView()
{
	Q_D(qSlicerFacetedVisualizerModuleWidget);

	QStandardItemModel *standardModel = new QStandardItemModel;
	QStandardItem *rootNode = standardModel->invisibleRootItem();

	std::list< std::string>::iterator it;
	for(it = queryLog.begin(); it != queryLog.end(); ++it)
	{
		QStandardItem *qItem = new QStandardItem(QString::fromStdString(*it));
		rootNode->appendRow(qItem);
	}
	d->treeViewQueryLog->setModel(standardModel);
	d->treeViewQueryLog->expandAll();

	 QItemSelectionModel *selectionModel = d->treeViewQueryLog->selectionModel();
	 connect (selectionModel, SIGNAL(selectionChanged(const QItemSelection &,
				  const QItemSelection &)), this,
				  SLOT(onQueryLogItemSelected(const QItemSelection &, const QItemSelection &)));
}


//---------------------------------------------------------------------------------
void qSlicerFacetedVisualizerModuleWidget::
updateAtlasNodesTree()
{

	Q_D(qSlicerFacetedVisualizerModuleWidget);

	QStandardItemModel *standardModel = new QStandardItemModel;
	QStandardItem *rootNode = standardModel->invisibleRootItem();

	for (unsigned int i = 0; i < this->unMatchedMRMLAtoms.size(); ++i)
	{
		QStandardItem *qItem = new QStandardItem(QString::fromStdString(unMatchedMRMLAtoms[i]));
		rootNode->appendRow(qItem);
	}
	d->treeViewAtlasModels->setModel(standardModel);
	d->treeViewAtlasModels->expandAll();

	QItemSelectionModel *selectionModel = d->treeViewAtlasModels->selectionModel();
	connect( selectionModel, SIGNAL(selectionChanged(const QItemSelection &,
			const QItemSelection&)), this,
			SLOT(onAtlasNodeSelectionChanged(const QItemSelection &, const QItemSelection &)));
}


void qSlicerFacetedVisualizerModuleWidget::
onAtlasNodeSelectionChanged(const QItemSelection&, const QItemSelection&)
{
	// find the index of the the selected node and show the corresponding DB Items
	Q_D(qSlicerFacetedVisualizerModuleWidget);
	const QModelIndex index = d->treeViewAtlasModels->selectionModel()->currentIndex();

	QString selectedText = index.data(Qt::DisplayRole).toString();
	// get the corresponding index for this text
	int indx = -1;
	for (unsigned i = 0; indx == -1 && i < this->unMatchedMRMLAtoms.size(); ++i)
	{
		if(this->unMatchedMRMLAtoms[i] == selectedText.toStdString())
		{
			indx = i;
		}
	}
	if(indx != -1)
	{
		this->selectedMRMLAtomIndex = indx;
	}
	// show the text in the atlas model area
	this->GetUserMatchesForMRMLTerms(selectedText.toStdString());
	this->onMRMLAtomChanged(selectedText);

}

//-----------------------------------------------------------------------------
// this is a helper for allowing syncing models with database. This is used when the
// user explictly sets which MRML model should be matched to the DB atom from the
// set of possible matches produced by the faceted search
void qSlicerFacetedVisualizerModuleWidget::
onMatchDBMRMLAtom()
{
	Q_D(qSlicerFacetedVisualizerModuleWidget);
	vtkSlicerFacetedVisualizerLogic *logic = d->logic();

	if(this->DBAtom != "none")
	{
	  logic->SetCorrespondingDBTermforMRMLNode(this->DBAtom, this->mrmlAtom);
	}

	this->readyNextMRMLDBAtomMatch = true;

	this->selectedMRMLAtomIndex++;

	if(selectedMRMLAtomIndex < this->unMatchedMRMLAtoms.size())
	{
	  this->GetUserMatchesForMRMLTerms(unMatchedMRMLAtoms[this->selectedMRMLAtomIndex]);
	}

}


//-----------------------------------------------------------------------------
// this is triggered when the user selects a matching DB item for a given MRML item
void qSlicerFacetedVisualizerModuleWidget::onMatchingDBItemSelected(const QItemSelection&,
		const QItemSelection& )
{

	Q_D(qSlicerFacetedVisualizerModuleWidget);
	const QModelIndex index = d->treeViewDBElements->selectionModel()->currentIndex();

	QString selectedText = index.data(Qt::DisplayRole).toString();

	this->DBAtom = selectedText.toStdString();
	d->lineEdit_DBatom->setText(selectedText);

}

//-----------------------------------------------------------------------------
void qSlicerFacetedVisualizerModuleWidget::onMRMLAtomChanged(const QString &text)
{

	Q_D(qSlicerFacetedVisualizerModuleWidget);

	this->mrmlAtom = text.toStdString();
	// set up the tree
	int indx = this->selectedMRMLAtomIndex;

	std::cout<<" UPDATING MATCHING DB TERMS FOR "<<text.toStdString()<<" matching atoms "<<
			this->matchingDBAtoms[indx].size()<<std::endl;

	QStandardItemModel *standardModel = new QStandardItemModel;
	QStandardItem *rootNode = standardModel->invisibleRootItem();

	QStandardItem *qItem = new QStandardItem(text);

	for (unsigned j = 0; j < this->matchingDBAtoms[indx].size(); ++j)
	{
		QString item = QString::fromStdString(this->matchingDBAtoms[indx][j]);
		QStandardItem *resItem = new QStandardItem(item);
		qItem->appendRow(resItem);
	}
	QString lastItem = "none";
	QStandardItem *resItem = new QStandardItem(lastItem);
	qItem->appendRow(resItem);
	rootNode->appendRow(qItem);
	d->treeViewDBElements->setModel(standardModel);
	d->treeViewDBElements->expandAll();

	QItemSelectionModel *selectionModel = d->treeViewDBElements->selectionModel();
	connect(selectionModel, SIGNAL(selectionChanged(const QItemSelection&, const QItemSelection &)),
			this, SLOT (onMatchingDBItemSelected(const QItemSelection&, const QItemSelection& )));

}


//-----------------------------------------------------------------------------
void qSlicerFacetedVisualizerModuleWidget::
GetUserMatchesForMRMLTerms(std::string unmatched)
{

	Q_D(qSlicerFacetedVisualizerModuleWidget);

	this->readyNextMRMLDBAtomMatch = false;
	d->lineEdit_mrml->setText(QString::fromStdString(unmatched));
}

